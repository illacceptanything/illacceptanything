import rng
import requests
import shutil

# Get the number of the latest xkcd
current_xkcd_num = int(requests.get('http://xkcd.com/info.0.json').json()['num'])

# Generate a random number using rng.py (expecting a natural number)
# Mod it by current_xkcd_num to give us the number of a valid xkcd comic
random_xkcd_num = rng.get_random_number() % current_xkcd_num

# Get the URL of the image of the random comic
img_url = requests.get('http://xkcd.com/' + str(random_xkcd_num) + '/info.0.json').json()['img']

# Get the random comic and save it as 'dank.png'
response_img = requests.get(img_url, stream=True)
with open('dank.png', 'wb') as out_file:
    shutil.copyfileobj(response_img.raw, out_file)