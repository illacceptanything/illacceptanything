import os
import sys
import time
from sys import platform as _platform

class colors:
	red = '\033[31m'  # red
	blue = '\033[34m'  # blue
	magenta = '\033[35m'  # purple
	cyan = '\033[36m'  # cyan
	reset = "\033[0m"

if _platform == "linux" or _platform == "linux2":
      print'[!] You are on Linux.'
      print colors.cyan + '[1] Who is my master?'
      print colors.magenta + '[2] Who do I listen to?'
      print colors.reset
      
      master_command = raw_input('\nGive me a command: ')
      if master_command == '1':
            os.system('say You are my master.')
            os.system('whoami')
      elif master_command == '2':
            os.system('say I listen to you my master.')
      else:
            os.system('say Shutting down.')
            time.sleep(3)
            sys.exit()
            
elif _platform == "darwin":
      print'[!] You are on OSX.'
      print colors.cyan + '[1] Who is my master?'
      print colors.magenta + '[2] Who do I listen to?'
      print colors.reset
      
      master_command = raw_input('\nGive me a command: ')
      if master_command == '1':
            os.system('say You are my master.')
            os.system('whoami')
      elif master_command == '2':
            os.system('say I listen to you my master.')
      else:
            os.system('say Shutting down.')
            time.sleep(3)
            sys.exit()
