#include <stdlib.h>

int main() {
  int* a = malloc(sizeof(int) * 2);
  a[0] = 5;
  a[1] = 10;

  int b[2];
  b[0] = 15;
  b[1] = 20;

  int* x = a;

  int y = a[1];
}
