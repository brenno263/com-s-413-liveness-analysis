#include <stdio.h>

// Dead function
int dead_func() {
   int a = 0;
   char b = 'b';
   printf("This function contains dead code as it will never be run. %c", b);

   return a;
}

// Another dead function
int dead_func2() {
   int f = dead_func();

   return f;
}

int main(int argc, char *argv[]) {
    return 1;
}