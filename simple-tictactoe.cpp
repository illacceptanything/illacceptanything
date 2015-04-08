/* Name: Shah Syed
 * Start Date: August 24th, 2014
 * End Date: August 25th, 2014
 * A simple 2 Player tic tac toe program in C++
 * http://www.shahsyed.com
*/

#include <iostream>
#include <string>
#include <vector>
#include <limits>

class gameboard
{
public: 
	char c1 = '1', c2 = '2', c3 = '3', c4 = '4', c5 = '5', c6 = '6', c7 = '7', c8 = '8', c9 = '9';
	int playerNum = 1;
	int boardPos = 0;
	bool winCondition = false;
	bool quit = false;

	void play()
	{
		std::cout << "Welcome to simple-tictactoe!" << std::endl;
		while (winCondition == false)
		{
			drawBoard();
			std::cout << "Player " << playerNum << "'s turn:" << std::endl;
			std::cout << "Choose a number corresponding to where on the board you want to play:" << std::endl;
			// Check if input is only integers
			while (!(std::cin >> boardPos))
			{
				std::cin.clear();
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				std::cout << "Please enter integers only:" << std::endl;
			}
			while (legal(playerNum, boardPos) == false)
			{
				std::cout << "The move by Player " << playerNum << " is illegal." << std::endl;
				std::cout << "Choose a number corresponding to where on the board you want to play:" << std::endl;
				// Check if input is only integers
				while (!(std::cin >> boardPos))
				{
					std::cin.clear();
					std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					std::cout << "Please enter integers only:" << std::endl;
				}
			}
			if (win(playerNum) == true)
			{
				drawBoard();
				break;
			}
			if (draw() == true)
			{
				drawBoard();
				break;
			}
			else if (playerNum == 1)
				playerNum = 2;
			else if (playerNum == 2)
			{
				playerNum = 1;
			}
		}
		
	}
	// drawBoard the game board
	void drawBoard()
	{
		std::cout << "| " << c1 << " | " << c2 << " | " << c3 << " |" << std::endl;
		std::cout << "-------------" << std::endl;
		std::cout << "| " << c4 << " | " << c5 << " | " << c6 << " |" << std::endl;
		std::cout << "-------------" << std::endl;
		std::cout << "| " << c7 << " | " << c8 << " | " << c9 << " |" << std::endl;
	}
	// Check if the move is legal
	bool legal(int playerNum, int boardPos)
	{
		if (playerNum == 1)
		{
			if (boardPos == 1 && isdigit(c1))
			{
				c1 = 'x';
				return true;
			}
			else if (boardPos == 2 && isdigit(c2))
			{
				c2 = 'x';
				return true;
			}
			else if (boardPos == 3 && isdigit(c3))
			{
				c3 = 'x';
				return true;
			}
			else if (boardPos == 4 && isdigit(c4))
			{
				c4 = 'x';
				return true;
			}
			else if (boardPos == 5 && isdigit(c5))
			{
				c5 = 'x';
				return true;
			}
			else if (boardPos == 6 && isdigit(c6))
			{
				c6 = 'x';
				return true;
			}
			else if (boardPos == 7 && isdigit(c7))
			{
				c7 = 'x';
				return true;
			}
			else if (boardPos == 8 && isdigit(c8))
			{
				c8 = 'x';
				return true;
			}
			else if (boardPos == 9 && isdigit(c9))
			{
				c9 = 'x';
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (playerNum == 2)
		{
			if (boardPos == 1 && isdigit(c1))
			{
				c1 = 'o';
				return true;
			}
			else if (boardPos == 2 && isdigit(c2))
			{
				c2 = 'o';
				return true;
			}
			else if (boardPos == 3 && isdigit(c3))
			{
				c3 = 'o';
				return true;
			}
			else if (boardPos == 4 && isdigit(c4))
			{
				c4 = 'o';
				return true;
			}
			else if (boardPos == 5 && isdigit(c5))
			{
				c5 = 'o';
				return true;
			}
			else if (boardPos == 6 && isdigit(c6))
			{
				c6 = 'o';
				return true;
			}
			else if (boardPos == 7 && isdigit(c7))
			{
				c7 = 'o';
				return true;
			}
			else if (boardPos == 8 && isdigit(c8))
			{
				c8 = 'o';
				return true;
			}
			else if (boardPos == 9 && isdigit(c9))
			{
				c9 = 'o';
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}
	// Check if the player has won
	bool draw()
	{
		if (!(isdigit(c1)) && !(isdigit(c2)) && !(isdigit(c3)) && !(isdigit(c4)) && !(isdigit(c5)) && !(isdigit(c6)) && !(isdigit(c7)) && !(isdigit(c8)) && !(isdigit(c9)))
		{
			std::cout << "The game is a draw! How exciting!" << std::endl;
			return true;
		}
		return false;
	}
	bool win(int playerNum)
	{
		if ((c1 == c2) && (c1 == c3) && (c2 == c3))
		{
			std::cout << "Win Condition 1 met" << std::endl;
			std::cout << "Player " << playerNum << " wins!" << std::endl;
			return true;
		}
		else if ((c1 == c5) && (c1 == c9) && (c5 == c9))
		{
			std::cout << "Win Condition 2 met" << std::endl;
			std::cout << "Player " << playerNum << " wins!" << std::endl;
			return true;
		}
		else if ((c1 == c4) && (c1 == c7) && (c4 == c7))
		{
			std::cout << "Win Condition 3 met" << std::endl;
			std::cout << "Player " << playerNum << " wins!" << std::endl;
			return true;
		}
		else if ((c2 == c5) && (c2 == c8) && (c5 == c8))
		{
			std::cout << "Win Condition 4 met" << std::endl;
			std::cout << "Player " << playerNum << " wins!" << std::endl;
			return true;
		}
		else if ((c3 == c6) && (c3 == c9) && (c6 == c9))
		{
			std::cout << "Win Condition 5 met" << std::endl;
			std::cout << "Player " << playerNum << " wins!" << std::endl;
			return true;
		}
		else if ((c3 == c5) && (c3 == c7) && (c5 == c7))
		{
			std::cout << "Win Condition 6 met" << std::endl;
			std::cout << "Player " << playerNum << " wins!" << std::endl;
			return true;
		}
		else
		{
			return false;
		}
	}
};

int main()
{
	gameboard game;
	game.play();
	return 0;
}
