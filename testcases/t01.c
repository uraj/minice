extern int print_int(int x);
extern int print_string(char* x);

int test_const(int* a)
{
  int c1,c2,c3;
  c1 = 1; c2 = 2; c3 = 3;
  a[1] = 5;
  a[a[c3-c1*c2]] = 1;
  print_int (a[5]);
  print_string ("\nresult\n");
  print_int ( c3 * c2 );
}

int main()
{
  int a[6], i;
  for (i = 0; i< 6; i++)
    a[i]=i;
  test_const(a);
}
