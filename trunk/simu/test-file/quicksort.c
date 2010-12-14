#ifdef UNICORE
void __minic_print(int arg)
{
    return;
}
#else
#include <stdio.h>
#endif
void quicksort(int * beg, int * end)
{
    if(end <= beg)
        return;
    int * left = beg;
    int * right = end;
    int temp = *left;
    while(left < right)
    {
        while(left < right)
        {
            if(*right < temp)
            {
                *left = *right;
                ++left;
                break;
            }
            else
                --right;
        }
        while(left < right)
        {    
            if(*left > temp)
            {
                *right = *left;
                --right;
                break;
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
    int a[9] = {5,2,4,3,8,1,6,9,7};
    
    quicksort(a, a + 8);
#ifdef UNICORE
    for(j = 0; j < 8; ++j)
        __minic_print(a[j]);
#else
    for(j= 0; j < 8; ++j)
        printf("%d\n", a[j]);
#endif
    return 0;
}

