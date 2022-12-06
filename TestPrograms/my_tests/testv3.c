#include <stdio.h>
int main() {
  int max;
  scanf("%d", &max);

  for (int i = 0; i < max; i++) {
    if (i % 2 == 0) {
      printf("fizz");
    }
    if (i % 3 == 0) {
      printf("buzz");
    }
    if (i % 5 == 0) {
      printf("baz");
    }
    if (i % 7 == 0) {
      printf("bang");
    }
    printf("\n");
  }

  if (max < 40) {
    printf("the max was less than 40, too!");
  }
  return 0;
}
