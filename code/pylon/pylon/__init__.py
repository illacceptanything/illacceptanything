import sys
import os
import math
import glob
import random
import atexit
from time import sleep

do_remove = True
pylon_name = ''

def remove_pylon():
    if do_remove:
        os.remove(pylon_name)

def construct():
    global do_remove
    global pylon_name
    logical_step = 0.042 # Anything other than fastest is heresy
    build_time = 450
    progress_squares = 34
    health = 300
    shields = 300
    pylon_name = 'pylon-' + str(random.getrandbits(64))

    atexit.register(remove_pylon)

    sys.stdout.write('Protoss Pylon\n\n')

    for n in range(build_time):
        sys.stdout.write('\r')
        sys.stdout.write(str(int(math.ceil((n/build_time)*health))))
        sys.stdout.write('/' + str(health))
        sys.stdout.write(' ')
        sys.stdout.write(str(int(math.ceil((n/build_time)*shields))))
        sys.stdout.write('/' + str(shields))
        sys.stdout.write(' ')
        sys.stdout.write('O'*int(math.ceil((n/build_time)*progress_squares)))

        f = open(pylon_name, 'w')
        f.write('Protoss Pylon\n\n')
        f.write(str(int(math.ceil((n/build_time)*health))))
        f.write('/' + str(health))
        f.write(' ')
        f.write(str(int(math.ceil((n/build_time)*shields))))
        f.write('/' + str(shields) + '\n')
        f.close()
        sleep(logical_step)

    sys.stdout.write('\r')
    sys.stdout.write(' '*(12+2+2+34))
    sys.stdout.write('\r')

    pylons = glob.glob('pylon-*')

    total = len(pylons)*8
    total = total if total < 200 else 200
    used = len(glob.glob('*')) - len(pylons)
    used = used if used > 0 else 0

    f = open(pylon_name, 'w')
    f.write('Protoss Pylon\n\n')
    f.write(str(int(math.ceil((n/build_time)*health))))
    f.write('/' + str(health))
    f.write(' ')
    f.write(str(int(math.ceil((n/build_time)*shields))))
    f.write('/' + str(shields))
    f.write('\n\n')
    f.write('Psi Used: ' + str(used) + '\n')
    f.write('Psi Provided: 8\n')
    f.write('Total Psi: ' + str(total) + '\n')
    f.write('Psi Max: 200\n')
    f.close()

    sys.stdout.write(str(int(math.ceil((n/build_time)*health))))
    sys.stdout.write('/' + str(health))
    sys.stdout.write(' ')
    sys.stdout.write(str(int(math.ceil((n/build_time)*shields))))
    sys.stdout.write('/' + str(shields))
    sys.stdout.write('\n\n')
    sys.stdout.write('Psi Used: ' + str(used) + '\n')
    sys.stdout.write('Psi Provided: 8\n')
    sys.stdout.write('Total Psi: ' + str(total) + '\n')
    sys.stdout.write('Psi Max: 200\n')
    do_remove = False

