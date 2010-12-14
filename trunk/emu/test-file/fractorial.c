void __minic_print(int arg)
{
    return;
}

int fractor(int n)
{
    if(n == 0 || n == 1)
        return 1;
    return n * fractor(n - 1);
}

int main()
{
    __minic_print(fractor(10));
    return 0;
}
