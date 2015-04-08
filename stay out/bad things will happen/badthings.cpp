#include <iostream>

int main()
{
	int j = 1;
	while(j){
		std::cout << "Bad things are currently happening." << std::endl;
		if(j++ == ((1LL << 10) / 24))
			break;
	};

	return 0;
}
