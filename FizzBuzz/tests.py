import unittest
import fizzbuzz

# We have to have tests naturally

class FizzBuzzTest(unittest.TestCase):

    def test_some_fizz(self):
        self.assertEqual(fizzbuzz.fizz_buzz_wonder(lambda: 'Fizz'), 'Fizz!')

    def test_some_buzz(self):
        self.assertEqual(fizzbuzz.fizz_buzz_wonder(lambda: 'Buzz'), 'Buzz!')

    def test_some_fizz_buzz(self):
        self.assertEqual(fizzbuzz.fizz_buzz_wonder(lambda: 'Hi!'), 'FizzBuzz!')