# Tic-Tac-Toe Game


# Create a dictionary to store the structure of the tic-tac-toe board
theBoard = {'top-l': '', 'top-m': '', 'top-r': '',
			'mid-l': '', 'mid-m': '', 'mid-r': '',
			'bottom-l': '', 'bottom-m': '', 'bottom-r': ''}

# Function to print the layout of the board on the screen
def printBoard(board):
	print('\n\t ' + board['top-l'] + ' | ' + board['top-m'] + ' | ' + board['top-r'])
	print('\t--+--+--')
	print('\t ' + board['mid-l'] + ' | ' + board['mid-m'] + ' | ' + board['mid-r'])
	print('\t--+--+--')
	print('\t ' + board['bottom-l'] + ' | ' + board['bottom-m'] + ' | ' + board['bottom-r'] + '\n')


# Function to check if game has been won or not
def checkWin(board):
	if(board['top-l'] == board['top-m'] == board['top-r'] == 'X' or 
		board['mid-l'] == board['mid-m'] == board['mid-r'] == 'X' or
		board['bottom-l'] == board['bottom-m'] == board['bottom-r'] == 'X' or 
		board['top-l'] == board['mid-l'] == board['bottom-l'] == 'X' or
		board['top-m'] == board['mid-m'] == board['bottom-m'] == 'X' or
		board['top-r'] == board['mid-r'] == board['bottom-r'] == 'X' or
		board['top-l'] == board['mid-m'] == board['bottom-r'] == 'X' or
		board['top-r'] == board['mid-m'] == board['bottom-l'] == 'X'):

		return 'X'

	elif(board['top-l'] == board['top-m'] == board['top-r'] == 'O' or 
		board['mid-l'] == board['mid-m'] == board['mid-r'] == 'O' or
		board['bottom-l'] == board['bottom-m'] == board['bottom-r'] == 'O' or 
		board['top-l'] == board['mid-l'] == board['bottom-l'] == 'O' or
		board['top-m'] == board['mid-m'] == board['bottom-m'] == 'O' or
		board['top-r'] == board['mid-r'] == board['bottom-r'] == 'O' or
		board['top-l'] == board['mid-m'] == board['bottom-r'] == 'O' or
		board['top-r'] == board['mid-m'] == board['bottom-l'] == 'O'):

		return 'O'

	else:
		return ''


# Messages to show before game starts
print('\tLet the match BEGIN!!!!!!!\n')
print('***********************************************************************')
# How to Play Instructions
print('To make a move specify (top, mid OR bottom) along with a "-" followed by (l, m OR r)')
print('For Exmaple: To make a move in the middle box specify "mid-m"\n')
print('***********************************************************************')
print('\tSTART!!!!!!\n')

# X starts the game
turn = 'X'

# win is initialized to empty
win = ''

# prints the initial blank board
printBoard(theBoard)

# The loop runs 9 times as only at max 9 moves can occur in the game
for i in range(1, 10):
	move = input('Turn for ' + turn + '. Move on which space?: ')
	# Fills the specified space in the board with the 'X' or 'O'
	theBoard[move] = turn
	# Updates the board display by printing the new updated board
	printBoard(theBoard)

	if turn == 'X':
		turn = 'O'
	else:
		turn = 'X'

	# To check if a winning pattern has occured
	win = checkWin(theBoard)

	if(win == 'X' or win == 'O'):
		break

if(win != ''):
	print('The winner is ' + win + '!')
else:
	print('The game is a draw!')
