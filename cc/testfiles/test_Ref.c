int a, *b, c[10], *d;
int main()
{
	int e, *f, g[10], *h;
	b = &e;
	b = &a;
	
	f = &e;
	f = &a;
	
	b = &c[5];
	b = &g[5];

	f = c;
	f = g;
	f = &c[5];
	f = &g[5];

	e = (*b);
	e = (*f);

	a = (*b);
	a = (*f);
}
