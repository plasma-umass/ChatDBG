#include <iostream>

void doSomething(int * ptr) {
  *ptr = 0;
}
  
int main()
{
  int * n = new int(100);
  n--;
  delete n;
  char * ch = new char[16];
  delete [] ch;
  doSomething(n);
  std::cout << "n = " << *n << std::endl;
  return 0;
}
