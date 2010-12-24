/* test brace */

extern int printline_int(int x);

int main()
{
  int a[3];
  int x, y, z;
  x = 1;
  y = 2;
  a[0] = 3;
  a[1] = (a[0]) + 1;
  a[2] = a[(1)] + 1;
  z = 1 + (2 + (a[0]+4)) + (a[1]==2);
  printline_int(z); 
}

