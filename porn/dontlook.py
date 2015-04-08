with open('porn2.png', 'wb') as f:
    f.write(open('porn.png', 'rb').read().replace('memes', 'a'))
    f.close()
