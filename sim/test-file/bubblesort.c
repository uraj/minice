extern int printline_int(int x);
extern int a[10000];

int main()
{
    int i, j, temp;
    for(i = 10000; i >0; --i)
    {
        for(j = 0; j < i; ++j)
            if(a[j] > a[j+1])
            {
                temp = a[j+1];
                a[j+1] = a[j];
                a[j] = temp;
            }
    }
    for(j = 0; j < 10000; ++j)
        printline_int(a[j]);
    return 0;
}

