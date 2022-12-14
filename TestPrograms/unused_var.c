#include <stdio.h>
int main() {
  int a = 3;
  int b = 5;
  int c;
  int d = 4;
  int x = 100; // x is set and not used.

  if (a > b) {
	c = a + b; // here c is also set and not used.
	d = 2;
  }
  c = 4;

  printf("%d", c);
  return b * d + c;
}
