#include <iostream>
using namespace std;

int ternarysearch(int *a, int left, int right, int target){
	int mid1 = left + (right - left)/3;
	int mid2 = left + 2*(right - left)/3;
	if (target == a[mid1]){
		return mid1;}
	else if (target == a[mid2]){
		return mid2;}
	else if (mid1 >= mid2){
		return -1;}
	if (target < a[mid1]){
		return ternarysearch(a, left, mid1-1, target);}
	if (target > a[mid2]){
		return ternarysearch(a, mid2+1, right, target);}
	else {
		return ternarysearch(a, mid1+1, mid2-1, target);}
}
