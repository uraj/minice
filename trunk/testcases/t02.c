/* Test While */
extern int print_int(int x);
extern int print_string(int x);

int main()
{
  int i;
  i = 0;
  while ( i < 10 || (i != 10 && (i+3) < 100))
  {
    i = i + 1;
    print_int(i);
    print_string("\n");
  }
  for ( i = 0; i < 10 || (i==10)&& ((i+3)<100); i++)
  {
    print_int(i);
    print_string("\n");
  }
  return 0;
}

