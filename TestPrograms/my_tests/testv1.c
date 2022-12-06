#include <stdio.h>
int main() {
  int max;
  scanf("%d", &max);


  int value = 0;
  if (value == 1) {
	printf("asdf");
  }

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
    printf("\n");
  }
  return 0;
}
