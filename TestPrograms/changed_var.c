#include <stdio.h>

int main(int argc, char *argv[]) {
   int c = 0;
   // c is changed
   c += 1;

   // Code is accessed
   if (c == 1) {
      printf("c = 1");
   }

   return 1;
}