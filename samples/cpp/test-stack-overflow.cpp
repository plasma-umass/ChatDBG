#include <iostream>

int fib(int n) {
  return fib(n-1) + fib(n-2);
}

int main()
{
  auto const n = 100;
  std::cout << "fib(" << n << ") = " << fib(n);
  return 0;
}
