int a1()
{
     int i,j,k;
     j=0;
     for(i<j ; i=j ; (i=i+j)&&(i>j))
     {
          k = i + j;
     }
     return 0;
}

void a2()
{
     int i,j,k, *p, *m;
     int a[10];
     
     if(i == 0)
          p = &i;
     else
          p = &j;
     k = *p;
     (*p) = 11;
     m = p;
     i = (*m);
     (*m) = j;
     *(p + 1) = j;
     i = *(m + 1);

     k = a[0];
     a[1] = 1;
     i = a[2];
     a[3] = 3;
     j = a[1] + a[3];
     *(a + 2) = 2;
     k = *(a + 4);

	 for(i=0 ; i<10 ; i++)
     {
          j = i;
          k = i+j;
     }
     k=i;
     return;
}
