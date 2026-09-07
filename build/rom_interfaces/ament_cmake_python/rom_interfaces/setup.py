from setuptools import find_packages
from setuptools import setup

setup(
    name='rom_interfaces',
    version='1.0.0',
    packages=find_packages(
        include=('rom_interfaces', 'rom_interfaces.*')),
)
