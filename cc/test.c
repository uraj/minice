int b()
{
	int k;
	k =  k + 1;
	return k;
}

int loop()
{
	int a, c;
	
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
			b();
		}
	}

	if(a)
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
