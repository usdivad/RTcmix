/* a modification of unix rand() to return floating point values between
   + and - 1. */

static	long	randx = 1;

void
srrand(unsigned x)
{
	randx = x;
}


float rrand()
{
	int i = ((randx = randx*1103515245 + 12345)>>16) & 077777;
	return((float)i/16384. - 1.);
}
