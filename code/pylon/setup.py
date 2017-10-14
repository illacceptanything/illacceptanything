from setuptools import setup

setup(name='pylon',
      version='13.37',
      description='Construct additional pylons on the command line',
      url='https://github.com/illacceptanything/illacceptanything/tree/master/code/pylon',
      license='MIT',
      packages=['pylon'],
      entry_points = {'console_scripts': ['pylon=pylon:construct']})
