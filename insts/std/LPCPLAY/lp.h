/*										LP.H	*/

/* constants used throughout lpc code */

#define	LP_MAGIC    999
#define	MAXPOLES    64
#define	MAXFRAME    (MAXPOLES + 4)
#define	LPBUFSIZ    4096
#define	LPBUFVALS   1024
#define	BUFNOSHIFT  10
#define	BUFPOSMASK  1023
/* indices into lpc data array */
#define RESIDAMP     0
#define RMSAMP       1
#define THRESH       2
#define PITCH        3

#ifdef __cplusplus
extern "C" {
#endif

/*
   Declarations of all functions which might be used globally
*/
double shift(float, float, float);
void bmultf(float *array, float mult, int number);
float buzz(float amp, float si, float hn, float *f, float *phs);
int checkForHeader(int afd, int *nPoles, float sr);

/* temporary until compiler bug fixed */
void l_srrand(unsigned x);
void l_brrand(float amp, float *a, int j);

#ifdef __cplusplus
}
#endif
