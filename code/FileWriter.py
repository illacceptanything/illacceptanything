from sys import argv

script, filename = argv

print "We're going to erase %r." % filename
print "If you don't want that, type 2."
print "If you do want that, type 1."

user_input = int(raw_input("> "))
if user_input == 2:
	print("Keeping file...\nClosing now... Goodbye!")

elif user_input == 1:
	print "Opening file..."
	target = open(filename, 'w')

	print "Emptying file...\nFile emptied..."
	target.truncate()
	

	print "Please write three lines of text."
	line1 = raw_input()
	line2 = raw_input()
	line3 = raw_input()

	print "Writing file..."
	target.write("%r\n%r\n%r" %(line1, line2, line3))

	print "Closing now... Goodbye!"
	target.close()

else:
	print("Invalid command entered... Goodbye!")
