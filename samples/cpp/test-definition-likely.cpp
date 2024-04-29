static int* p = nullptr;

struct Bob
{
  int ****data;
};
struct Adam
{
  Bob *b1;
  Bob *b2;
};

int main()
{
  int **p2 = &p;
  int ***p3 = &p2;
  int ****p4 = &p3;

  Bob bob1 = {p4};
  Bob bob2 = {p4};
  Adam adam1 = {&bob1, &bob2};

  int n = ****adam1.b1->data;
  return 0;
}
