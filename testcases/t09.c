/* test arguments */
extern int printline_char(char x);
extern int printline_int(char x);

int func1(int x, int y, char z)
{
  if (x != y)
    printline_char(z);
   
  return x+y;
}

int main()
{
  printline_int(func1(1,2,"Y"));
}
