#include <stdio.h>
int main() {
  int a = 3;
  int b = 5;
  int c;
  int d = 4;
  int x = 100;

  // Here C is used before being initialized
  printf("%d", c);

  if (a > b) {
	d = 2;
  }
  c = 4;

  printf("%d", c);
  return b * d + c;
}
