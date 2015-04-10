#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int main() {
    int n;
    time_t t;
    srand((unsigned) time(&t));
    n = rand() % 4 + 1;
    printf("Enter a recursive acronym: ");
    getchar();
    sleep(n);
    getchar(); /* throw away newlines */
    printf("not enough memory to complete the operation");
    getchar();
    printf("\n");
    return 0;
}
