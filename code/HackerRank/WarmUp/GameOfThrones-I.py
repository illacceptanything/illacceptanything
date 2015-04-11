def count(char, string):
    count = 0
    for i in string:
        if (i is char):
            count += 1
    return count
            
n = input()
length = len(n) 
counter = 0
numOfOdd = 0
for i in set(n):
    temp = count(i, n)
    if (temp % 2 == 1):
        numOfOdd += 1
    counter += temp
    
if (counter == length and numOfOdd <= 1):
    print ("YES")
else:
    print ("NO")
    
    