#include <iostream>
using namespace std;

int x[] = { 1, 2, 3, 4, 5 };

void foo(int n, float b) {
  cout << "TEST " << x[n * 10000] << endl;
}

int main()
{
  for (auto i = 0; i < 10; i++) {
    foo(i, 1.0);
  }
  return 0;
}
