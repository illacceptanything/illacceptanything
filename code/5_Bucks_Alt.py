import random
import sys
import datetime

time = datetime.datetime.now() #Set up the seed based on the clock.
table = [] #The table of words in the input.
output = [] #The table of words in the output.
final = "\n " #The final output.
k = 0
line = 0

random.seed = time

print( "Enter Text. \n" )

for inp in sys.stdin:
    for word in inp.split():
        table.append( word )
  
for i in range(len(table)):
    j = i + random.randint(-3, 3)
    
    if j > len(table) - 1: #Prevents the index from going out of range.
        j = len(table) - 1
    
    if j != k: #Prevents the same word from being selected twice.
        output.append( table[j] )
        if j - 1 != -1:
            output.append( table[j-1] )
    
    k = j
    
for j in range(len(output)):
    final = final + (output[j] + " ")
    line = line + 1
    if line == 10: # Line length.
        final = (final + "\n \n")
        line = 0

print( "Output: \n")
print( final )
