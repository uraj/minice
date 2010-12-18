#include <string.h>
#include <stdio.h>

void __minic_print(int a)
{
    printf("%d", a);
    return;
}

int main()
{
	char * str1, * str2;
    
	int a[201],b[200],len1,len2,i,l;
    l = 0;
    str1 = "83651738957938175";
    str2 = "2432985087917398479173";
    
	a[200]=0;
	for(i=0;i<200;i++)
	{
        a[i]=0;
        b[i]=0;
	}
    len1 = strlen(str1);
    len2 = strlen(str2);
	for(i=0;i<len1;i++)
        a[i]=str1[len1-1-i]-48;
    for(i=0;i<len2;i++)
        b[i]=str2[len2-1-i]-48;
	for(i=0;i<200;i++)
	{
        a[i]=a[i]+b[i];
        if(a[i]>=10)
        {
            a[i]=a[i]-10;
            a[i+1]=a[i+1]+1;
        }
	}
	for(i=200;i>=0;i--)
        if(a[i]!=0)
        {
            l=i;
            break;
        }
	for(i=l;i>=0;i--)
        __minic_print(a[i]);
	return 0;
}
