#include <assert.h>
#include <iostream>

void f(int value) {
  // The value in this function is expected to be in [1, 5].
  std::cout << value << std::endl;
}

int main() {
  int a[5] = {1, 2, 3, 4, 5};
  int b[5] = {6, 7, 8, 9, 10};

  f(*(a + 5));
}
