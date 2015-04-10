def palindrome(x):
  x = str(x)
  return len(x) < 2 or (x[0] == x[-1] and palindrome(x[1:-1]))
  
def is_palindrome(word):
  """ :returns: True if <word> is a palindrome
      :param str word: a string
  """
  return word == word[::-1]
