/* Test Expressions */ 

extern int printline_int(int x);

int main()
{
  int a, b, c;
  a = 1;
  b = 2;
  c = 3;
  printline_int( ((((((a)))))) + b * a);
  c = + a + (+b);
  printline_int(c);
  c = -a - (-b);
  printline_int(c);
  return 0;
}
