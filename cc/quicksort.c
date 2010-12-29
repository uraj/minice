extern int printline_int(int x);

void quicksort(int * beg, int * end)
{ 
    int * left;
    int * right;
    int temp;
	int out_loop;
	
	left = beg;
	right = end;
	temp = *left;
	
	if(end <= beg)
        return;
    while(left < right)
    {
		out_loop = 1;
        while(left < right && out_loop == 1)
        {
            if(*right < temp)
            {
                *left = *right;
				++left;
				out_loop = 0;
            }
            else
                --right;
        }
        while(left < right && out_loop == 0)
        {    
            if(*left > temp)
            {
                *right = *left;
                --right;
				out_loop = 1;
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
    int a[100];
    a[0] = 479;
    a[1] = 218;
    a[2] = 304;
    a[3] = 704;
    a[4] = 331;
    a[5] = 190;
    a[6] = 198;
    a[7] = 826;
    a[8] = 952;
    a[9] = 979;
    a[10] = 27;
    a[11] = 878;
    a[12] = 156;
    a[13] = 871;
    a[14] = 257;
    a[15] = 937;
    a[16] = 67;
    a[17] = 188;
    a[18] = 674;
    a[19] = 114;
    a[20] = 635;
    a[21] = 30;
    a[22] = 976;
    a[23] = 539;
    a[24] = 41;
    a[25] = 45;
    a[26] = 821;
    a[27] = 381;
    a[28] = 989;
    a[29] = 316;
    a[30] = 47;
    a[31] = 821;
    a[32] = 886;
    a[33] = 703;
    a[34] = 877;
    a[35] = 217;
    a[36] = 245;
    a[37] = 75;
    a[38] = 395;
    a[39] = 197;
    a[40] = 54;
    a[41] = 422;
    a[42] = 75;
    a[43] = 562;
    a[44] = 646;
    a[45] = 332;
    a[46] = 851;
    a[47] = 713;
    a[48] = 873;
    a[49] = 525;
    a[50] = 179;
    a[51] = 508;
    a[52] = 908;
    a[53] = 155;
    a[54] = 48;
    a[55] = 301;
    a[56] = 200;
    a[57] = 221;
    a[58] = 682;
    a[59] = 542;
    a[60] = 889;
    a[61] = 81;
    a[62] = 363;
    a[63] = 775;
    a[64] = 784;
    a[65] = 240;
    a[66] = 993;
    a[67] = 29;
    a[68] = 667;
    a[69] = 740;
    a[70] = 227;
    a[71] = 73;
    a[72] = 515;
    a[73] = 654;
    a[74] = 635;
    a[75] = 161;
    a[76] = 987;
    a[77] = 486;
    a[78] = 874;
    a[79] = 860;
    a[80] = 363;
    a[81] = 53;
    a[82] = 368;
    a[83] = 623;
    a[84] = 560;
    a[85] = 768;
    a[86] = 924;
    a[87] = 760;
    a[88] = 989;
    a[89] = 606;
    a[90] = 654;
    a[91] = 878;
    a[92] = 687;
    a[93] = 17;
    a[94] = 6;
    a[95] = 823;
    a[96] = 609;
    a[97] = 351;
    a[98] = 205;
    a[99] = 276;
    quicksort(a, a + 99);
    for(j = 0; j < 100; ++j)
        printline_int(a[j]);
    return;
}

