#!/usr/bin/env python
import random

words = {'jet': '1',
         'fuel': '2',
         'can\'t': '3',
         'melt': '4',
         'dank': '5',
         'memes': '6'}

meme = []


def random_sort(meme):
    memes = list(words.items())
    random.shuffle(memes)
    for key, value in memes:
        meme.append(str(key))

    meme = ' '.join(meme)

    return meme


if __name__ == "__main__":
    print(random_sort(meme))
