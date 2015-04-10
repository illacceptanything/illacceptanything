"""
A crypto engine for the 21st century.
"""

import sys
import string

sigma = {chr(65+i): i for i in range(26)}
rotation = None
plain = ''
cypher = ''
codes = []
newCodes = []
cypherList = []
while rotation is None:
    try:
        rotation = int(input("Choose a number for your cypher: "))
    except ValueError:
        rotation = int(input("Please choose a number. The number may be positive or negative: "))
        continue
if rotation % 26 == 0:
    cont = input('This operation will return your message exactly. Continue? [y]es or [n]o: ')
    if cont == 'y':
        pass
    elif cont == 'n':
        print("Exiting...")
        sys.exit()
elif rotation > 26:
    print('The maximum rotation is 26; your number will be treated as if it were {0}.'\
            .format(str(rotation % 26)))
mod = rotation % 26
plain = input("Choose a message: ").upper()
for i in plain:
    if i == ' ':
        codes.append(' ')
    elif i in string.punctuation:
        codes.append(i)
    else:
        codes.append(sigma[i])

for i in codes:
    if type(i) is int:
        newCodes.append((i + mod) % 26)
    else:
        newCodes.append(i)

for i in newCodes:
    if type(i) is int:
        cypherList.append(chr(65+i))
    else:
        cypherList.append(i)

cypher = ''.join(cypherList)
print(cypher)
