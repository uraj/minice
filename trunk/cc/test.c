int g_a2;
int g_a1;
int b(int a1, int a2, int a3, int a4, int a5);

int b(int a1, int a2, int a3, int a4, int a5)
{
	int k;
	int *p;
	k = a1;
	k = a2;
	k = a3;
	k = a4;
	k = a5;
	k =  k + g_a2;
	p = &g_a1;
	g_a2 = g_a2 + g_a1;	
	if(a1 > 1)
	{
		return k + 1;
	}
	else
	{
		return k;
	}
	while(1)
	{
		k= k + g_a1;
	}
}

int loop()
{
	int a, c;
	int * d, * e;
	if(a > c || a < c)
	{
		a = c;
	}
	else
	{
		if(a < c)
		{
			c = a++;
		}
		else
		{
			a = c++;
		}
	}
	
	while(a > c)
	{
		while(a < c)
		{
			b(a, c, a, c, a);
		}
	}

	a = a + 1121231;
	if(12321424)
	{
		for(a; a; a ++)
		{
			while(a > c)
			{
				return 0;
			}
		}
	}
	else
	{
		if(a < c)
		{
			return 1;
		}
	}
}
