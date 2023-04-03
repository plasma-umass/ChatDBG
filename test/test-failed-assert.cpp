#include <assert.h>
#include <iostream>

float fact(float n) {
  auto x = 1.0;
  for (auto i = 0.0; i < n; i++) {
    x *= i;
  }
  assert(x != 0.0);
  return x;
}


int main()
{
  std::cout << fact(100) << std::endl;
  return 0;
}
