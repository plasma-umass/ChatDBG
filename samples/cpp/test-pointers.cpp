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
  int num = 1;
  int *p1 = &num;
  int **p2 = &p1;
  int ***p3 = &p2;
  int ****p4 = &p3;

  Bob bob1 = {p4};
  Bob bob2 = {nullptr};
  Adam adam1 = {&bob1, &bob2};

  int n = ****adam1.b2->data;
  return 0;
}
