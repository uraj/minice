int a1()
{
     int i,j,k;
     i=1;
     j=0;
     for(i<j ; i=j ; (i=i+j)&&(i>j))
     {
          k = i + j;
     }
     return 0;
}

void a2()
{
     int i,j,k;
     for(i=0 ; i<10 ; i++)
     {
          j = i;
          k = i+j;
     }
     k=i;
     return;
}
