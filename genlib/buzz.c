#define  ABS(x) ((x < 0) ? (-x) : (x))
#define EPS .1e-06

float buzz(float amp, float si, float hn, float *f, float *phs)
{
	register j,k;
	float q,d,h2n,h2np1;
	j = *phs;
	k = (j+1) % 1024;
	h2n = 2. * hn; 
	h2np1 = h2n + 1.; 
	q = (int)((*phs - (float)j) * h2np1)/h2np1;
	d = *(f+j);
	d += (*(f+k)-d)*q;
	if(ABS(d) < EPS) q = amp;
          else { 
	    k = (long)(h2np1 * *phs) % 1024;
	    q = amp * (*(f+k)/d - 1.)/h2n;
	    } 
	*phs += si;
	while(*phs >= 1024.)  
		*phs -= 1024.;
	return(q);
}
