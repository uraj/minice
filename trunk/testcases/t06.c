/* Test array */

extern int printline_int(int x);

int a[4];

int main()
{
  int b[2];
  int i;

  for (i=0;i<2;i++)
    b[i] = i+8;
  for (i=0;i<4;i++)
    a[i] = i+1;

  printline_int(b[a[0]]);
  printline_int(a[a[0]] + b[a[1]+1]);
}
