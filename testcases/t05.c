/* Test Function Call */

extern int printline_int(int x);

int func2()
{
  printline_int (3);
  return 0;
}

int func1()
{
  printline_int(2);

  func2();

  printline_int(4);
}

int func3()
{
  func1();
}

int main()
{
  func3();
}
