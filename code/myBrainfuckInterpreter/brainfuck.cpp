#include <iostream>
#include <stack>
using namespace std;

char mem[30000];
int idx = 0;
stack<unsigned int> loopout; 
int steps;

int main()
{
	string input;
	cin >> input;

	for(int curr = 0;curr < input.size();curr ++)
	{
		switch(input[curr])
		{
			case '+':
				mem[idx] ++;
				break;
			case '-':
				mem[idx] --;
				break;
			case '>':
				idx ++;
				break;
			case '<':
				idx --;
				break;
			case '.':
				cout << mem[idx];
				break;
			case ',':
				cin >> mem[idx];
				break;
			case '[':
				if(mem[idx]!=0)
				{
					//Continue looping
					loopout.push(curr);
				}
				else
				{
					int open = 1;
					int closed = 0;
					for(int i = curr+1;i < input.size() || open!=closed;i ++)
					{
						if(input[i] == '[')
							open ++;
						if(input[i] == ']')
							closed ++;
						if(open == closed)
						{
							curr = i;
							break;
						}
					}
				}
				break;
			case ']':
				curr = loopout.top() - 1;
				loopout.pop();
				break;
		}
		//steps ++;
	}
	//cout << "Finished! " << steps << " steps." << endl;
}
