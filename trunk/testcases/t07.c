/* test recursion */

extern int printline_int(int x);

int limit;

void rr (int res)
{
  res = res + 1;
  printline_int(res);
  if (res < limit)
    rr(res);
}

int main()
{
  int res;
  limit = 10;
  res = 1;
  rr (res);
}
