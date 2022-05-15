#include <stdio.h>
int main()
{
    int i, j, n, k = 0;

    printf("Enter a number to define the rows: ");
    scanf("%d", &n);

    for (i = 1; i <= n; i++)
    {
        for (j = 0; j <= n - i; j++)
        {
            printf(" ");
        }
        for (k = 1; k <= i; k++)
        {
            printf(" *");
        }
        printf("\n");
    }
}
