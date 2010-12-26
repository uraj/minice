int a;
int main(int b, int c, int d, int e, int f)
{
	int g;
	int *h;
	int i[10];
	char *j;
	char k[10];
	b = a + g;
	a = b + g;
	g = b + a;

	f = a + g;
	a = f + g;
	g = f + a;

	f = f + 1;
	a = a + 1;
	g = g + 1;

	f = 1 + f;
	a = 1 + a;
	g = 1 + g;

	g = (g + f) + (g + e);

	h = h + 1;
	h = 1 + h;
	h = i + 1;
	h = 1 + i;
	h = j + 1;
	h = 1 + j;
	h = k + 1;
	h = 1 + k;
}
