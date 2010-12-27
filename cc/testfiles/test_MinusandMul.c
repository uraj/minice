int a, *b, c[10];
char d, *e, f[10];
int main()
{
	int g, *h, i[10];
	char j, *k, l[10];
	/* test imme */
	a = g - 1;
	a = 1 - g;
	g = a - 1;
	g = 1 - a;

	/* test pointer */
	b = h - 1;
	h = b - 1;
	e = k - 1;
	k = e - 1;
	b = c - 1;
	e = f - 1;
	h = i - 1;
	k = l - 1;

	b = c - a;
	h = i - g;

	/* test mul */
	a = a * 1;
	g = 1 * g;
	a = a * g;
	a = (a * g) * (g * a);
}
