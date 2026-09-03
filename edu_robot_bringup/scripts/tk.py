#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import tkinter as tk
from threading import Thread

class ROS2Publisher(Node):
    def __init__(self):
        super().__init__('ros2_switch_publisher')
        self.publisher_ = self.create_publisher(String, 'output_topic', 10)
        self.get_logger().info('ROS 2 node started, ready to publish messages.')

    def publish_message(self, mode):
        msg = String()
        msg.data = mode
        self.publisher_.publish(msg)
        self.get_logger().info(f'Published: "{msg.data}"')

class TkROS2SwitchApp:
    def __init__(self, ros_node):
        self.ros_node = ros_node

        # Create the main window
        self.root = tk.Tk()
        self.root.title("ROS 2 Switch")

        # Create buttons
        self.button_navi = tk.Button(self.root, text="Navigation", command=self.switch_to_navi)
        self.button_mapping = tk.Button(self.root, text="Mapping", command=self.switch_to_mapping)
        self.button_remapping = tk.Button(self.root, text="Remapping", command=self.switch_to_remapping)

        # Pack the buttons into the window
        self.button_navi.pack(pady=10)
        self.button_mapping.pack(pady=10)
        self.button_remapping.pack(pady=10)

    def run(self):
        # Run the Tkinter main loop
        self.root.mainloop()

    def switch_to_navi(self):
        self.ros_node.publish_message("navi")

    def switch_to_mapping(self):
        self.ros_node.publish_message("mapping")

    def switch_to_remapping(self):
        self.ros_node.publish_message("remapping")

def ros2_spin(node):
    rclpy.spin(node)

def main(args=None):
    # Initialize the ROS 2 Python client library
    rclpy.init(args=args)

    # Create a ROS 2 node instance
    ros_node = ROS2Publisher()

    # Start the ROS 2 node in a separate thread
    ros_thread = Thread(target=ros2_spin, args=(ros_node,), daemon=True)
    ros_thread.start()

    # Create and run the Tkinter application
    app = TkROS2SwitchApp(ros_node)
    app.run()

    # Shutdown the ROS 2 library when done
    ros_node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
