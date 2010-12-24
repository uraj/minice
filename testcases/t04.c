/* Test if else */
extern int printline_int(int x);
int main()
{
  int a, b;
  a = 1;
  b = 3;
  if ( a >  2)
    if ( a )
      if (0)
        printline_int(1);
      else
        printline_int(2);
    else
      printline_int(3);

  a = 6;
  if ( b > a)
    if ( b > a+1)
      printline_int(4);
    else
      printline_int(5);
  else
    if (a == b + 3)
      if (b<4)
        printline_int(6);
      else
        printline_int(7);
          
  return 0;
}
