struct David
{
  int *data;
};
struct Charlie
{
  David *d1;
};
struct Bob
{

  Charlie *c1;
  Charlie *c2;
  David *d2;
};
struct Adam
{
  Bob *b1;
  Bob *b2;
};

int main()
{
  int arrayofints[] = {21, 24, 85, 56, 37, 10, 34, 48, 92, 13};
  int *p = &arrayofints[0];
  David david1 = {&arrayofints[4]};
  David david2 = {&arrayofints[2]};
  David david3 = {&arrayofints[8]};
  Charlie charlie1 = {&david1};
  Charlie charlie2 = {&david2};
  Bob bob1 = {&charlie1, &charlie2, &david3};
  David david4 = {&arrayofints[7]};
  David david5 = {nullptr};
  David david6 = {&arrayofints[0]};
  Charlie charlie3 = {&david4};
  Charlie charlie4 = {&david5};
  Bob bob2 = {&charlie3, &charlie4, &david6};
  Adam adam1 = {&bob1, &bob2};

  int n = *adam1.b2->c2->d1->data;
  return 0;
}
