#include <iostream>
#include <string>

using namespace std;

int main () {
	cout << "Hello, Human! Please prove your input into my port." << endl;
	string input;
	getline(cin, input);
	cout << "WHOA WHOA WHOA slow down there (guy / gal)! That's inappropriate!" << endl;
	cout << "Please ask any question to the greatest AI on the planet..." << endl;
	getline(cin, input);
	cout << "Your answer...42" << endl;
}