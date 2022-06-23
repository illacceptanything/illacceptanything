import colorama
from colorama import Fore, Back, Style
colorama.init()

print(Fore.RED + "keli5 was here!" + Style.RESET_ALL)
print(Back.YELLOW + Fore.BLUE + "Stand wit", end="")
print(Back.BLUE + Fore.YELLOW + "h Ukraine!" + Style.RESET_ALL)

colorama.deinit()
