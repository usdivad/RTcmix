/* Envelope Class, Perry R. Cook, 1995-96
   This is the base class for envelopes. This one is capable of ramping
   state from where it is to a target value by a rate. It also responds
   to simple KeyOn and KeyOff messages, ramping to 1.0 on keyon and to
   0.0 on keyoff. There are two tick (update value) methods, one returns
   the value, and other returns 0 if the envelope is at the target value
   (the state bit).
*/

#include "Envelope.h"    


Envelope :: Envelope(double srate) : _sr(srate)
{
   target = (MY_FLOAT) 0.0;
   value = (MY_FLOAT) 0.0;
   rate = (MY_FLOAT) 0.001;
   state = ENV_HOLDING;
}


Envelope :: ~Envelope()
{
}


void Envelope :: keyOn()
{
   target = (MY_FLOAT) 1.0;
   if (value != target)
      state = ENV_RAMPING;
}


void Envelope :: keyOff()
{
   target = (MY_FLOAT) 0.0;
   if (value != target)
      state = ENV_RAMPING;
}


void Envelope :: setRate(MY_FLOAT aRate)
{
   if (aRate < 0.0) {
      fprintf(stderr, "Negative rates not allowed! Correcting...\n");
      rate = -aRate;
   }
   else
      rate = aRate;
}


void Envelope :: setTime(MY_FLOAT aTime)
{
   if (aTime < 0.0) {
      fprintf(stderr, "Negative times not allowed! Correcting...\n");
      rate = (1.0 / _sr) / -aTime;
   }
   else
      rate = (1.0 / _sr) / aTime;
}


void Envelope :: setTarget(MY_FLOAT aTarget)
{
   target = aTarget;
   if (value != target)
      state = ENV_RAMPING;
}


void Envelope :: setValue(MY_FLOAT aValue)
{
   state = ENV_HOLDING;
   target = aValue;
   value = aValue;
}


int Envelope :: getState()
{
   return state;
}


MY_FLOAT Envelope :: tick()
{
   if (state == ENV_RAMPING) {
      if (target > value) {
         value += rate;
         if (value >= target) {
            value = target;
            state = ENV_HOLDING;
         }
      }
      else {
         value -= rate;
         if (value <= target) {
            value = target;
            state = ENV_HOLDING;
         }
      }
   }
   return value;
}


int Envelope :: informTick()
{
   this->tick();
   return state;
}


MY_FLOAT Envelope :: lastOut()
{
   return value;
}


