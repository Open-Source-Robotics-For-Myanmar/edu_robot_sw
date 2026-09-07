import os
import math
import glob
import yaml
import time
import cv2
import re
import subprocess
import threading
import rclpy
from rclpy.node import Node
from rclpy.executors import MultiThreadedExecutor
from rclpy.callback_groups import ReentrantCallbackGroup
from sensor_msgs.msg import Image
from geometry_msgs.msg import Twist, PoseStamped
from cv_bridge import CvBridge
from nav2_simple_commander.robot_navigator import BasicNavigator, TaskResult
from rom_interfaces.srv import WhichVoiceCommand

import google.generativeai as genai
from PIL import Image as PILImage

class GeminiRobotManager(Node):
    def __init__(self):
        super().__init__('gemini_robot_manager')

        # Reentrant callback group - service callback ထဲမှာ blocking call သုံးနိုင်ရန်
        self.reentrant_group = ReentrantCallbackGroup()

        # 1. Gemini Model Placeholder (API Key ကို service call တိုင်းတွင် အသစ်ပြန်ယူပါမည်)
        self.model = None

        # 2. ROS 2 Initializations
        self.bridge = CvBridge()
        self.latest_frame = None
        self.navigator = BasicNavigator()
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)

        # Camera Subscriber
        self.create_subscription(
            Image, '/camera1/image_raw', self.subscribeCamera1, 10,
            callback_group=self.reentrant_group
        )

        # Service Server
        self.srv = self.create_service(
            WhichVoiceCommand,
            'which_voice_command',
            self.which_voice_command_callback,
            callback_group=self.reentrant_group
        )

        # ROS 2 parameter ကိုအသုံးပြု၍ Hardcode ပြဿနာဖြေရှင်းခြင်း
        self.declare_parameter('waypoint_path', '/home/buc_robot/data/waypoints/')
        self.waypoint_path = self.get_parameter('waypoint_path').get_parameter_value().string_value
        self.get_logger().info("Gemini Robot Manager is Ready...")

    def subscribeCamera1(self, msg):
        """Camera Data ကို အမြဲ Update လုပ်နေမည့် function"""
        self.latest_frame = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')

    def speakVoiceResponse(self, text):
        """Speaker ကနေ အသံထွက်မည့် function (TTS)"""
        self.get_logger().info(f"Speaking: {text}")
        # Blocking မဖြစ်စေရန် subprocess.Popen အသုံးပြုထားခြင်း
        subprocess.Popen(['espeak', text])

    def get_all_waypoints_data(self):
        """YAML file အားလုံး၏ content ကို စုစည်းခြင်း"""
        files = glob.glob(os.path.join(self.waypoint_path, "*.yaml"))
        context = "Available Waypoints and their coordinates:\n"
        for f in files:
            with open(f, 'r') as stream:
                data = yaml.safe_load(stream)
                context += f"{os.path.basename(f)}: {data}\n"
        return context

    def which_voice_command_callback(self, request, response):
        """Service Main Logic - synchronous callback (MultiThreadedExecutor သုံးထားလို့ block ဖြစ်လည်းရ)"""
        self.get_logger().info(
            f"[DEBUG] =====> Service called! Received audio_data size: {len(request.audio_data)} bytes, "
            f"format: {request.format}, request_action: {request.request_action} <====="
        )
        
        # Service call တိုင်းအတွက် GEMINI_API_KEY အသစ်ပြန်ယူခြင်း
        api_key = os.getenv('GEMINI_API_KEY')
        if not api_key:
            self.get_logger().error("GEMINI_API_KEY not found in environment!")
            response.success = False
            response.action_type = "chat"
            response.error_msg = "GEMINI_API_KEY not set"
            return response
            
        genai.configure(api_key=api_key)
        # အသုံးပြုသူ လိုချင်သော Custom model ကို သုံးခြင်း
        self.model = genai.GenerativeModel('models/gemini-robotics-er-1.6-preview')

        wav_data = request.audio_data

        # ၁။ Gemini ဆီ ပို့ရန် Prompt အရင်ဆုံး ဆုံးဖြတ်ခြင်း (Intent Classification)
        audio_part = {"mime_type": "audio/wav", "data": bytes(wav_data)}

        intent_prompt = (
            "Analyze this audio. Is the user asking to: "
            "1. navigate to a place, 2. search for something, or "
            "3. search and go to an object? Reply with only 'NAV', 'SEARCH', or 'SEARCH_GO'."
        )
        
        # API Error Handling
        try:
            intent_res = self.model.generate_content([intent_prompt, audio_part]).text.strip()
            self.get_logger().info(f"Gemini intent classification response: '{intent_res}'")
        except Exception as e:
            self.get_logger().error(f"Gemini API Error (Intent): {e}")
            response.success = False
            response.action_type = "chat"
            response.error_msg = f"API Error: {e}"
            return response

        # --- Scenario 1: Navigation ---
        if "NAV" in intent_res:
            waypoint_context = self.get_all_waypoints_data()
            nav_prompt = (
                f"{waypoint_context}\n Based on the audio, which location name of "
                f"which YAML file should I use? Return only the position and direction "
                f"of that location as x,y,theta."
            )
            
            try:
                target_location = self.model.generate_content([nav_prompt, audio_part]).text.strip()
                self.get_logger().info(f"Gemini navigation response: '{target_location}'")

                self.get_logger().info(f"Navigating to {target_location}")
                nav_success = self.execute_nav(target_location)
                response.success = nav_success
                response.action_type = "navigate"
                response.action_params = target_location
                response.response_text = target_location  # Gemini ရဲ့ တကယ့် response စာသား
            except Exception as e:
                self.get_logger().error(f"Gemini API Error (Nav): {e}")
                response.success = False
                response.action_type = "navigate"
                response.response_text = "There was an error communicating with the brain."
                response.error_msg = f"API Error: {e}"

        # --- Scenario 2 & 3: Search / Search & Go ---
        elif "SEARCH" in intent_res:
            is_go = "SEARCH_GO" in intent_res
            found, search_gemini_text = self.perform_360_search(audio_part, is_go)
            response.success = found
            response.action_type = "find_object" if is_go else "search"
            response.response_text = search_gemini_text  # Gemini ရဲ့ တကယ့် response စာသား

        else:
            # --- Scenario 4: General Chat (Unknown Intent) ---
            self.get_logger().info("Intent was not NAV or SEARCH. Falling back to General Chat.")
            chat_prompt = "You are a helpful, friendly robot assistant. Please respond to the user's audio naturally and briefly."
            try:
                chat_res = self.model.generate_content([chat_prompt, audio_part]).text.strip()
                self.get_logger().info(f"Gemini chat response: '{chat_res}'")
                
                response.success = True
                response.action_type = "chat"
                response.response_text = chat_res
                self.speakVoiceResponse(chat_res)
            except Exception as e:
                response.success = False
                response.action_type = "chat"
                response.response_text = intent_res  # အကယ်၍ error တက်လျှင် မူလ intent ကို ပြပေးရန်
                response.error_msg = f"Unknown intent: {intent_res} and Chat API Error: {e}"

        self.get_logger().info(f"[DEBUG] Final Response Text: {response.response_text}")
        return response

    def perform_360_search(self, audio_part, is_go):
        """၃၆၀ ဒီဂရီ လှည့်ပြီး ရှာဖွေခြင်း"""
        self.get_logger().info("Starting 360 degree search...")
        start_time = time.time()
        twist = Twist()
        twist.angular.z = 0.5  # ဖြည်းဖြည်းလှည့်ရန်

        api_result = {"found": False, "done": False, "gemini_res": ""}

        def call_gemini():
            while not api_result["done"] and (time.time() - start_time) < 12.5:
                if self.latest_frame is not None:
                    # Thread အချင်းချင်း data မငြိစေရန် copy ယူခြင်း
                    frame_copy = self.latest_frame.copy()
                    cv2_img = cv2.cvtColor(frame_copy, cv2.COLOR_BGR2RGB)
                    pil_img = PILImage.fromarray(cv2_img)

                    search_prompt = (
                        "Looking at this image and listening to the audio, "
                        "do you see the object mentioned? Reply only 'FOUND' or 'NOT_FOUND'."
                    )
                    
                    try:
                        res = self.model.generate_content([search_prompt, audio_part, pil_img]).text.strip()
                        self.get_logger().info(f"Gemini search response: '{res}'")
                        api_result["gemini_res"] = res  # နောက်ဆုံးရထားသည့် အဖြေကို မှတ်ထားရန်

                        if "FOUND" in res:
                            api_result["found"] = True
                            api_result["done"] = True
                            break
                    except Exception as e:
                        self.get_logger().error(f"Gemini API Error (Search): {e}")

                time.sleep(1.0)  # တစ်စက္ကန့်တစ်ခါ Gemini ကို မေးမည်
            api_result["done"] = True

        # API ခေါ်ယူခြင်းကို Background Thread တွင်ထားရှိမည်
        t = threading.Thread(target=call_gemini)
        t.start()

        # cmd_vel ကို 10Hz နှုန်းဖြင့် အမြဲ publish မည်။ (Robot ရပ်မသွားစေရန်)
        while rclpy.ok() and not api_result["done"] and (time.time() - start_time) < 12.5:
            self.cmd_vel_pub.publish(twist)
            time.sleep(0.1)

        # ပြီးဆုံးပါက Thread အား ရပ်စောင့်ပြီး Robot ကို ရပ်မည်
        api_result["done"] = True
        t.join(timeout=2.0)
        self.cmd_vel_pub.publish(Twist())

        if api_result["found"]:
            self.speakVoiceResponse("I found the object you were looking for!")
            if is_go:
                self.get_logger().info("Object found, moving towards it...")
                # ဤနေရာတွင် Object ရှိရာသို့ ချဉ်းကပ်သည့် logic ထည့်နိုင်သည်
            return True, api_result["gemini_res"]
        else:
            self.speakVoiceResponse("I could not find the object.")
            return False, api_result["gemini_res"]

    def execute_nav(self, coordinate_str):
        """Gemini ဆီကလာတဲ့ 'x,y,theta' string ကို Nav2 goal အဖြစ်ပြောင်းလဲခြင်း"""
        try:
            # ၁။ String ကို Parse လုပ်ပြီး float values များအဖြစ်ပြောင်းခြင်း
            # Regex အသုံးပြု၍ LLM စာအပိုများပါလာခြင်းကို ဖြေရှင်းခြင်း
            numbers = re.findall(r'-?\d+\.?\d*', coordinate_str)
            if len(numbers) >= 3:
                target_x = float(numbers[0])
                target_y = float(numbers[1])
                target_theta = float(numbers[2])
            else:
                raise ValueError("Could not extract 3 coordinates (x, y, theta)")

            self.get_logger().info(
                f"Navigating to X:{target_x}, Y:{target_y}, Theta:{target_theta}"
            )
            self.speakVoiceResponse("Understood. I am moving to the requested location.")

            # ၂။ Goal Pose တည်ဆောက်ခြင်း
            goal_pose = PoseStamped()
            goal_pose.header.frame_id = 'map'
            goal_pose.header.stamp = self.get_clock().now().to_msg()

            # Position
            goal_pose.pose.position.x = target_x
            goal_pose.pose.position.y = target_y

            # ၃။ Theta (Euler) ကို Quaternion အဖြစ်ပြောင်းခြင်း (Z-axis rotation)
            goal_pose.pose.orientation.z = math.sin(target_theta / 2.0)
            goal_pose.pose.orientation.w = math.cos(target_theta / 2.0)

            # ၄။ Nav2 ဆီ Goal ပို့ခြင်း
            self.navigator.goToPose(goal_pose)

            # ၅။ Task ပြီးဆုံးသည်အထိ စောင့်ကြည့်ခြင်း
            # CPU အလွန်အကျွံအသုံးပြုမှုကို ကာကွယ်ရန် time.sleep() ထည့်သွင်းခြင်း
            while not self.navigator.isTaskComplete():
                time.sleep(0.1)

            result = self.navigator.getResult()
            if result == TaskResult.SUCCEEDED:
                self.get_logger().info("Navigation Succeeded!")
                self.speakVoiceResponse("I have reached the destination.")
                return True
            else:
                self.get_logger().error("Navigation Failed!")
                self.speakVoiceResponse("Navigation failed. Please check the path.")
                return False

        except Exception as e:
            self.get_logger().error(f"Error parsing coordinates: {e}")
            self.speakVoiceResponse("I received invalid coordinates from the brain.")
            return False


def main():
    rclpy.init()
    node = GeminiRobotManager()
    executor = MultiThreadedExecutor()
    executor.add_node(node)
    try:
        executor.spin()
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
