void __minic_print(int arg)
{
    return;
}

int test(int a)
{
    return a+1;
}

int main()
{
    int i, j, temp;
    int a[5] = {5,2,4,3,1};
    
    for(i = 5; i >0; --i)
    {
        for(j = 0; j < i; ++j)
            if(a[j] > a[j+1])
            {
                temp = a[j+1];
                a[j+1] = a[j];
                a[j] = temp;
            }
    }
    for(j = 0; j < 5; ++j)
        __minic_print(a[j]);
    return 0;
}

