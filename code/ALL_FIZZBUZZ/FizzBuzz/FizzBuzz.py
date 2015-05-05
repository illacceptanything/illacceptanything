def fizz_buzz_wonder(input_func):
    user_input = input_func()
    if user_input.lower() == 'fizz':
        return 'Fizz!'
    elif user_input.lower() == 'buzz':
        return 'Buzz!'
    else:
        return 'FizzBuzz!'

def prompt_for_input():
    return input('Enter "Fizz" or "Buzz": ').strip()

if __name__ == '__main__':
    print(fizz_buzz_wonder(prompt_for_input))
    print("Did I get the job?")