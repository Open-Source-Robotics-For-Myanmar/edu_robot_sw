import os
from setuptools import setup

package_name = 'gemini_robotics'

setup(
    name=package_name,
    version='1.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'),
            ['launch/gemini_robotics.launch.py']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='buc_robot',
    maintainer_email='server01.psa1981@gmail.com',
    description='Gemini AI powered voice command and navigation for ROM Robotics',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'activate_gemini_robotics = gemini_robotics.activate_gemini_robotics:main',
        ],
    },
)
