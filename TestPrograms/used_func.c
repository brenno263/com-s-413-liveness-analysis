#include <stdio.h>

int dead_func() {
   int a = 0;
   char b = 'b';
   printf("This function contains dead code as it will never be run. %c", b);

   return a;
}

int dead_func2() {
   int f = dead_func();

   return f;
}

int main(int argc, char *argv[]) {
   int c = dead_func2();
   
   return 1;
}