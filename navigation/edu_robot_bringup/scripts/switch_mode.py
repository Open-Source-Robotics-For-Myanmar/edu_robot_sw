#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import os
import subprocess
import time

from ament_index_python.packages import get_package_share_directory

class SwitchMode(Node):
    
    def __init__(self):
        super().__init__('switch_mode')
        self.current_process = None
        self.current_mode = None  

        description_pkg = get_package_share_directory('edu_robot_nav2')

        self.navigation_launch_file = os.path.join(description_pkg,'launch','navigation.launch.py')
        self.cartographer_launch_file = os.path.join(description_pkg,'launch','mapping.launch.py')
        self.remapping_launch_file = os.path.join(description_pkg,'launch','remapping.launch.py')

        # Create a subscriber
        self.subscription = self.create_subscription(
            String,
            'output_topic',
            self.mode_callback,
            10
        )

    def mode_callback(self, msg):
        new_mode = msg.data

        if new_mode == self.current_mode:
            self.get_logger().info(f"Mode is already {new_mode}. No change needed.")
            return

        if self.current_process is not None:
            self.get_logger().info(f"Shutting down current process for mode: {self.current_mode}")
            self.current_process.terminate()  
            self.current_process.wait()  
            self.current_process = None  
            self.get_logger().info(f"Process for {self.current_mode} terminated.")

        if new_mode == 'navi':
            self.get_logger().info('Switching to navigation mode...')
            self.current_process = subprocess.Popen(['ros2', 'launch', 'edu_robot_nav2', 'navigation.launch.py'])
            self.current_mode = new_mode

        elif new_mode == 'mapping':
            self.get_logger().info('Switching to mapping mode...')
            self.current_process = subprocess.Popen(['ros2', 'launch', 'edu_robot_nav2', 'mapping.launch.py'])
            self.current_mode = new_mode

        elif new_mode == 'remapping':
            self.get_logger().info('Switching to remapping mode...')
            self.current_process = subprocess.Popen(['ros2', 'launch', 'edu_robot_nav2', 'remapping.launch.py'])
            self.current_mode = new_mode

        self.get_logger().info(f"New mode '{new_mode}' process started.")

def main(args=None):
    rclpy.init(args=args)
    node = SwitchMode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Shutting down...")

    rclpy.shutdown()

if __name__ == '__main__':
    main()
