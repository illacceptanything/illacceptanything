from flask import Flask, jsonify

app = Flask(__name__)
app.config.from_object('config')


@app.route('/', methods=['GET'])
@app.route('/<teapot>', methods=['GET'])
def teapots(teapot=None):
    """This is where I come to make tea"""
    if teapot = None:
        teapot = 'teapot
    message = 'I\'m a little %s short and stout, here is my ' % teapot
    message += 'handle here is my spout. When you tip me over hear me '
    message += ' shout, HEY WHY DID YOU TIP ME OVER! COME BACK HERE!'

    data = {'teapot': message,
            }

    return jsonify(data)
