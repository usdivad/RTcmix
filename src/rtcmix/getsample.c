#include "../H/sfheader.h"
#include "../H/byte_routines.h"
#include <stdio.h>
#include <math.h>
#include "../H/ugens.h"


#if defined(NeXT) && defined(i386)
#include <architecture/byte_order.h>
#endif

extern SFHEADER sfdesc[NFILES];
extern int sfd[NFILES];
extern int swap_bytes[NFILES];
extern int bufsize[NFILES];
extern char *sndbuf[NFILES];
int (*getsample)();
int getfsample();
int getisample();

int
getsetnote(start,dur,filenum)
float start,dur;
int filenum;
{
	int nsamples = setnote(start,dur,filenum);
	_backup(filenum);
	if(sfclass(&sfdesc[filenum]) == SF_FLOAT) getsample = getfsample;
	else          	                          getsample = getisample;
	return(nsamples);

}

int
getisample(sampleno,c,input)
double sampleno;
float *c;
int input;
{

	int RECSIZE = bufsize[input];
	int BPREC = RECSIZE * sizeof(short);
	int BPFRAME = sfchans(&sfdesc[input]) * sizeof(short);
	int FPREC = RECSIZE/sfchans(&sfdesc[input]);

	int sample,i,j;
	signed short *array = (short *)sndbuf[input];
	float tempsample1;
	float tempsample2;
	float fraction;
	static int oldsample = 0;
	static int endsample = 0;

	sample = (int)sampleno;
	fraction = sampleno - (double)sample;
	if(!((sample >= oldsample) && (sample < endsample))) {
		if(sflseek(sfd[input], sample * BPFRAME, 0) <=0) {
			fprintf(stderr,"badlseek on inputfile\n");
			closesf();
		}
		if(read(sfd[input],(char *)array,BPREC) <= 0) {
			fprintf(stderr,"reached eof on input file \n");
			return(0);
		}
		oldsample = sample;
		endsample = oldsample + FPREC - 1;
		}
	for(i=(sample-oldsample)*sfchans(&sfdesc[input]),j=0; 
	    j<sfchans(&sfdesc[input]); i++,j++)  {
	  
	  if (swap_bytes[input]) {
	    tempsample1 = (signed short)reverse_int2(array+i);
	    tempsample2 = (signed short)reverse_int2(array+i+sfchans(&sfdesc[input]));
	    *(c+j) = tempsample1 + fraction * 
	      ((float)((signed short)tempsample2 - tempsample1));
	  }
	  else {
	    *(c+j) = (float)*(array+i) + fraction * 
	      ((float) *(array+i+sfchans(&sfdesc[input])) - 
	       (float) *(array+i));
	  }
	}
	return(1);

}

int
getfsample(sampleno,c,input)
double sampleno;
float *c;
int input;
{
	int RECSIZE = bufsize[input];
	int BPREC = RECSIZE * sizeof(float);
	int BPFRAME = sfchans(&sfdesc[input]) * sizeof(float);
	int FPREC = RECSIZE/(float)sfchans(&sfdesc[input]);

	int sample,i,j;
	float *array = (float *)sndbuf[input];
	float fraction,tempfloat1,tempfloat2;
	static int oldsample = 0;
	static int endsample = 0;
	extern float swapfloat();

	sample = (int)sampleno;
	fraction = sampleno - (double)sample;
	if(!((sample >= oldsample) && (sample < endsample))) {
		if(sflseek(sfd[input], sample * BPFRAME, 0) <=0) {
			fprintf(stderr,"badlseek on inputfile\n");
			closesf();
		}
		if(read(sfd[input],(char *)array,BPREC) <= 0) {
			fprintf(stderr,"reached eof on input file \n");
			return(0);
		}
		oldsample = sample;
		endsample = oldsample + FPREC - 1;
		}
	for(i = (sample-oldsample)*sfchans(&sfdesc[input]),j=0; 
					j<sfchans(&sfdesc[input]); i++,j++)  {
	  if (swap_bytes[input]) {
	    tempfloat1 = *(array+i);
	    byte_reverse4(&tempfloat1);
	    tempfloat2 = (*(array+i+sfchans(&sfdesc[input])));
	    byte_reverse4(&tempfloat2)
	      *(c+j) = tempfloat1 + fraction * (tempfloat2-tempfloat1);
	  }
	  else {
	    *(c+j) = *(array+i) + fraction * 
	      (*(array+i+sfchans(&sfdesc[input])) - *(array+i));
	  }
	return(1);
	}
}
