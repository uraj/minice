extern int printline_int(int x);
extern int a[10000];
void quicksort(int * beg, int * end)
{ 
  int * left;
  int * right;
  int temp;
  int out_loop;
	
  left = beg;
  right = end;
  temp = *left;
	
  if(end <= beg)
    return;
  while(left < right)
  {
    out_loop = 1;
    while(left < right && out_loop == 1)
    {
      if(*right < temp)
      {
        *left = *right;
        ++left;
        out_loop = 0;
      }
      else
        --right;
    }
    while(left < right && out_loop == 0)
    {    
      if(*left > temp)
      {
        *right = *left;
        --right;
        out_loop = 1;
      }
      else
        ++left;
    }   
  }
  *left = temp;
  quicksort(beg, left - 1);
  quicksort(left + 1, end);
  return;
}

int main()
{
  int j, temp;
  quicksort(a, a + 9999);
  for(j = 0; j < 10000; ++j)
    printline_int(a[j]);
  return;
}

