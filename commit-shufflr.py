import requests
from bs4 import BeautifulSoup
import random

def scrapr():
	resp = requests.get('https://github.com/mrkrstphr/illacceptanything')
	soup = BeautifulSoup(resp.text)
	filesList = soup.select('a.message')
	sentences = [item.string for item in filesList if item.string != None]
	words = []
	for sentence in sentences:
		words += sentence.split(' ')

	# get rid of extensions
	wordsNoExt = [word.split('.')[0] for word in words]
	random.shuffle(wordsNoExt)
	print " ".join(wordsNoExt)

if __name__ == '__main__':
	scrapr()