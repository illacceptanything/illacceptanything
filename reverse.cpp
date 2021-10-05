//Just a basic C++ program to reverse a number

// Reversing a no.
#include<iostream>
using namespace std;
int main(){
    int num;
    cout<<"Enter a no. ";
    cin>>num;
    int reversed = 0;
        while(num != 0) {
            int digit = num % 10;
            reversed = reversed * 10 + digit;
            num /= 10;
        }
    cout<<"Your reversed no. is "<<reversed<<endl;
  
  return 0;
}
