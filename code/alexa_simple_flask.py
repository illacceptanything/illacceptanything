import logging

from random import randint

from flask import Flask, render_template

from flask_ask import Ask, statement, question, session


app = Flask(__name__)

ask = Ask(app, "/")


@app.route('/')
def index():
    return "Yo, it is working!"

logging.getLogger("flask_ask").setLevel(logging.DEBUG)
quotes_list = [ "Batman says Why do we fall? So we can learn to pick ourselves back up.",
                "Spiderman says With great power, comes great responsibility.",
                "Green Lantern says No Matter how bad things get, something good is out there, over the horizon",
                "The Flash says Life does not give us purpose, We give life purpose.",
                "Batman says You only have your thoughts and dreams ahead of you. You are someone. You mean something.",
                "Batman says It is not who I am underneath, but what I do that defines me.",
                "Superman says There is a right and a wrong in the universe, and the distinction is not hard to make."   



                ]


fitness_list = [ "Exercise Daily",
					"Eat the Right Foods and Portion Each Meal",
					"Keep Track of Calories and Food Intake Per Day",
					"Be Sure to Get Sleep",
					"Stay Motivated"

				]

food_list = 	[ "Don't Drink Sugar Calories",
					"Despite being high in fat, nuts are incredibly nutritious and healthy.",
					"Avoid Processed Junk Food",
					"Coffee has been unfairly demonized. The truth is that it's actually very healthy.",
					"Take Care of Your Gut Health With Probiotics and Fiber"
				]


lifehack_list = [ 	"Meditate to clear your mind",
					" Take a stretch break",
					"Turn your commute into a chance to laugh",
					" Get a workout in while you watch your favorite show"

				]


yoga_list = 	[	" Create a comfortable spot for your yoga practice",
					"Always relax with savasana"
					"Practice regularly"
					"Varjasana is the best sitting position"

				]


@ask.launch

def new_game():

    welcome_msg = "Hello there, I'm Lively ! Want to hear some life hacks?"
    return question(welcome_msg)


@ask.intent("HelpIntent")
def help():
    return question("Hi there ! I will give you a life hack when you launch the skill or say yes or more tips. To launch this skill please say Alexa start lively. Was that helpful ?")
    
@ask.intent("AMAZON.HelpIntent")
def help():
    return question("Hi there ! I will give you a life hack when you launch the skill or say yes or more tips. To launch this skill please say Alexa start lively. Was that helpful ?")


@ask.intent("YesIntent")

def next_round():

    return question("Cool, now ask me something!")

@ask.intent("FitnessIntent")

def next_round1():

    return statement(fitness_list[randint(0,4)])

@ask.intent("FoodIntent")

def next_round2():

    return statement(food_list[randint(0,4)])

@ask.intent("YogaIntent")

def next_round3():

    return statement(yoga_list[randint(0,4)])

@ask.intent("MotivationIntent")

def next_round4():

    return statement(quotes_list[randint(0,4)])

@ask.intent("LifehackIntent")

def next_round5():

    return statement(lifehack_list[randint(0,4)])

# @ask.intent("AnswerIntent")

# def next_round():
    
#     return statement(quotes_list[randint(0,4)])



if __name__ == '__main__':

    app.run()
