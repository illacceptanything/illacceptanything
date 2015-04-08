def palindrome(x):
  x = str(x)
  return len(x) < 2 or (x[0] == x[-1] and palindrome(x[1:-1]))
