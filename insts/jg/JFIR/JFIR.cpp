/* JFIR - FIR filter, designed from a frequency response curve

   p0 = output start time
   p1 = input start time
   p2 = input duration
   p3 = amplitude multiplier
   p4 = filter order (higher order allows steeper slope)
   p5 = input channel [optional, default is 0]
   p6 = stereo spread (0 - 1) [optional, default is .5 for stereo output]
   p7 = bypass filter (0: no, 1: yes) [optional, default is 0]

   Can only process 1 channel at a time. To process stereo, call twice --
   once with inchan=0 and spread=1, again with inchan=1 and spread=0.
   
   Assumes function table 1 is amplitude curve for the note. (Try gen 18.)
   Or you can just call setline. If no setline or function table 1, uses
   flat amplitude curve.

   Function table 2 is the desired frequency response curve, described
   by freq,amp pairs. Frequency is in Hz, from 0 to Nyquist; amp is from
   0 to 1. Ideally, frequencies with amplitude of 1 are passed without
   attenuation; those with amplitude of 0 are attenuated totally. But
   this behavior depends on the order of the filter. Try an order of 200,
   and increase that as needed. (I've gotten an order of 600 in mono in
   real time on a PII266.)

   Example:

      nyquist = 44100 / 2
      makegen(2, 24, 5000, 0,0, 200,0, 300,1, 2000,1, 4000,0, nyquist,0)

   With a high order, this should attenuate everything below 200 Hz
   and above 4000 Hz.

   John Gibson (jgg9c@virginia.edu), 7/3/99.
   Filter design code adapted from Bill Schottstaedt's Snd.
*/
#include <stdio.h>
#include <stdlib.h>
#include <ugens.h>
#include <mixerr.h>
#include <Instrument.h>
#include "JFIR.h"
#include <rt.h>
#include <rtdefs.h>

//#define DEBUG
//#define PRINT_RESPONSE
#define NCOLUMNS 160      // 1 data point per terminal column
#define NROWS    60


JFIR :: JFIR() : Instrument()
{
   in = NULL;
}


JFIR :: ~JFIR()
{
   delete [] in;
   delete filt;
}


int JFIR :: init(float p[], int n_args)
{
   int   order, tabsize;
   float outskip, inskip, dur, ringdur;
   float *response_tab;
	int	rvin;

   outskip = p[0];
   inskip = p[1];
   dur = p[2];
   amp = p[3];
   order = (int)p[4];
   inchan = (int)p[5];
   spread = n_args > 6 ? p[6] : 0.5;             /* default is center */
   bypass = n_args > 7 ? (int) p[7] : 0;         /* default is no */

   ringdur = (float)order / SR;
   nsamps = rtsetoutput(outskip, dur + ringdur, this);
   rvin = rtsetinput(inskip, this);
	if (rvin == -1) { // no input
		return(DONT_SCHEDULE);
	}
   insamps = (int)(dur * SR);

   if (inchan >= inputchans) {
      die("JFIR", "You asked for channel %d of a %d-channel file.",
                                                         inchan, inputchans);
		return(DONT_SCHEDULE);
	}

   response_tab = floc(2);
   if (response_tab == NULL) {
      die("JFIR",
               "You haven't made the frequency response function (table 2).");
		return(DONT_SCHEDULE);
	}
   tabsize = fsize(2);

   if (order < 1) {
      die("JFIR", "Order must be greater than 0.");
		return(DONT_SCHEDULE);
	}

   filt = new NZero(order);
   filt->designFromFunctionTable(response_tab, tabsize, 0, 0);
#ifdef PRINT_RESPONSE
   print_freq_response();
#endif

   amparray = floc(1);
   if (amparray) {
      int lenamp = fsize(1);
      tableset(dur, lenamp, amptabs);
   }
   else
      advise("JFIR", "Setting phrase curve to all 1's.");

   skip = (int)(SR / (float)resetval);

   return nsamps;
}


int JFIR :: run()
{
   int   i, branch, rsamps;
   float aamp, insig;
   float out[2];

   if (in == NULL)              /* first time, so allocate it */
      in = new float [RTBUFSAMPS * inputchans];

   Instrument :: run();

   rsamps = chunksamps * inputchans;

   rtgetin(in, this, rsamps);

   aamp = amp;                  /* in case amparray == NULL */

   branch = 0;
   for (i = 0; i < rsamps; i += inputchans) {
      if (--branch < 0) {
         if (amparray)
            aamp = tablei(cursamp, amparray, amptabs) * amp;
         branch = skip;
      }
      if (cursamp < insamps)                 /* still taking input from file */
         insig = in[i + inchan];
      else                                   /* in ring-down phase */
         insig = 0.0;

      insig *= aamp;
      if (bypass)
         out[0] = insig;
      else
         out[0] = filt->tick(insig);

#ifdef DEBUG
printf("%4d:  %18.12f ->%18.12f ->%18.12f ->%18.12f\n",
       cursamp, insig, f1, f2, out[0]);
#else
      if (outputchans == 2) {
         out[1] = out[0] * (1.0 - spread);
         out[0] *= spread;
      }

      rtaddout(out);
#endif
      cursamp++;
   }

   return i;
}


void JFIR :: print_freq_response()
{
   float nyquist = SR * 0.5;
   float incr = nyquist / (float)NCOLUMNS;
   float *a, amps[NCOLUMNS], maxamp, rowamp;

   a = amps;
   for (float freq = 0.0; freq < nyquist; freq += incr)
      *a++ = filt->getFrequencyResponse(freq / nyquist);

   maxamp = 1.2;
   rowamp = maxamp / (float)NROWS;
   for (int row = NROWS; row > 0; row--) {
      maxamp -= rowamp;
      for (int col = 0; col < NCOLUMNS; col++) {
         if (amps[col] >= maxamp) {
            printf(".");
            amps[col] = -1.0;       // skip it from now on
         }
         else
            printf(" ");
      }
      printf("\n");
   }

#ifdef DEBUG
   printf("frequency response:\n");
   printf("freq: %8.2f   amp: %16.10f\n", freq, a);
#endif
}


Instrument *makeJFIR()
{
   JFIR *inst;

   inst = new JFIR();
   inst->set_bus_config("JFIR");

   return inst;
}


void
rtprofile()
{
   RT_INTRO("JFIR", makeJFIR);
}


