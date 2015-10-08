import os
import time
from sys import platform as _platform

class colors:
    red = '\033[31m' 
    blue = '\033[34m' 
    magenta = '\033[35m' 
    cyan = '\033[36m'
    reset = "\033[0m"

def main_master():

  if _platform == "linux" or _platform == "linux2":
      print'[!] You are on Linux.'
      print colors.cyan + '\n[1] Who is my master?'
      print colors.magenta + '[2] Who do I listen to?'
      print colors.reset
      
      master_command = raw_input('\nGive me a command: ')
      if master_command == '1':
            os.system('say You are my master.')
            os.system('whoami')
            main_master()
      elif master_command == '2':
            os.system('say I listen to you my master.')
            main_master()
      else:
            os.system('say Shutting down.')
            time.sleep(3)
            sys.exit()

  elif _platform == "darwin":
      print'[!] You are on OSX.'
      print colors.cyan + '\n[1] Who is my master?'
      print colors.magenta + '[2] Who do I listen to?'
      print colors.reset
      
      master_command = raw_input('\nGive me a command: ')
      if master_command == '1':
            os.system('say You are my master.')
            os.system('whoami')
            main_master()
      elif master_command == '2':
            os.system('say I listen to you my master.')
            main_master()
      else:
            os.system('say Shutting down.')
            time.sleep(3)
            sys.exit()

main_master()
