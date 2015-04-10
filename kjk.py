import random, math
from itertools import *

def respond(challenge, f, g):
    n = len(challenge)
    a = [f[challenge[i]] for i in range(0,n)]
    b = [0 for i in range(0,n)]
    b[0]=int(g[(a[0]+a[-1]) % 10])
    for i in range(1,n):
        b[i] = int(g[(b[i-1]+a[i]) % 10])
    return b

def checkg(g, pairs):
    f = {}
    for pair in pairs:
        n = len(pair[0])
        for i in range(1,n):
            if pair[0][i] not in f:
                f[pair[0][i]]=(int(g[g.index(pair[1][i])-1])-int(pair[1][i-1])) % 10
            elif f[pair[0][i]]!= ((int(g[g.index(pair[1][i])-1])-int(pair[1][i-1])) % 10):
                return False,f
        if pair[0][0] not in f:
            f[pair[0][0]]=(int(g[g.index(pair[1][0])-1])-f[pair[0][n-1]]) % 10
        elif f[pair[0][0]]!=((int(g[g.index(pair[1][0])-1])-f[pair[0][n-1]]) % 10):
            return False,f
    return True,f

def crackfg(pairs):
    templist = permutations('123456789')
    numperm = 0
    for gtemp in templist:
        g1='0'+''.join(gtemp)
        [check, f1] = checkg(g1,pairs)
        if check:
            f = f1
            g = g1
            numperm +=1
    print('#surviving permutations = ', numperm)
    return numperm, f, g

def passwordgame():
    pairs=[]
    more = 'Y'
    f={}
    g=''
    numperm=0
    sofar = ''
    print('Please use only lower case letters for challenges and digits for responses.')
    while more == 'Y' or more == 'y':
        challenge = input('Next Challenge? ')
        if numperm==1 and not list(filter(lambda x: x not in sofar, challenge)):
            print('Response = ', ''.join((str(x) for x in respond(challenge,f,g))))
        else:
            sofar = sofar+challenge
            response = input('Response? ')
            pairs.append([challenge, response])
            [numperm, f, g] = crackfg(pairs)
            print('A feasible character-to-digit map: ',f)
            print('A feasible digit permutation: ', g)
        if numperm > 0: 
            more = input('play again? (Y/N)')
        else: more = 'N'

passwordgame()
