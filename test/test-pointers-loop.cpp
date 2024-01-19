class Node
{
public:
  int data;
  Node *next;

  Node(int value) : data(value), next(nullptr) {}
};

int main()
{
  Node *node1 = new Node(10);
  Node *node2 = new Node(20);
  Node *node3 = new Node(30);

  node1->next = node2;
  node2->next = node1;

  Node n = *node3->next;

  delete node1;
  delete node2;
  delete node3;

  return 0;
}