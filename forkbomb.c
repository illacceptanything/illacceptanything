#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  for(;;) {
    printf("I'm forking you so hard.");
    fork();
  }
  return 0;
}
