/* One Zero Filter Class, by Perry R. Cook, 1995-96
   The parameter <gain> is an additional gain parameter applied to the
   filter on top of the normalization that takes place automatically.
   So the net max gain through the system equals the value of <gain>.
   <sgain> is the combination of <gain> and the normalization parameter,
   so if you set the zeroCoeff to alpha, sgain is always set to
   gain / (1.0 + fabs(alpha)).
*/

#include "OneZero.h"


OneZero :: OneZero() : Filter(0)
{
   gain = (MY_FLOAT) 1.0;
   zeroCoeff = (MY_FLOAT) 1.0;
   sgain = (MY_FLOAT) 0.5;
   inputs = new MY_FLOAT [1];
   this->clear();
}


OneZero :: ~OneZero()
{
   delete [] inputs;
}


void OneZero :: clear()
{
   inputs[0] = (MY_FLOAT) 0.0;
   lastOutput = (MY_FLOAT) 0.0;
}


void OneZero :: setGain(MY_FLOAT aValue)
{
   gain = aValue;
   if (zeroCoeff > 0.0)                  /* Normalize gain to 1.0 max  */
      sgain = gain / ((MY_FLOAT) 1.0 + zeroCoeff);
   else
      sgain = gain / ((MY_FLOAT) 1.0 - zeroCoeff);
}


void OneZero :: setCoeff(MY_FLOAT aValue)
{
   zeroCoeff = aValue;
   if (zeroCoeff > 0.0)                  /* Normalize gain to 1.0 max  */
      sgain = gain / ((MY_FLOAT) 1.0 + zeroCoeff);
   else
      sgain = gain / ((MY_FLOAT) 1.0 - zeroCoeff);
}


MY_FLOAT OneZero :: tick(MY_FLOAT sample)
{
   MY_FLOAT temp;
   temp = sgain * sample;
   lastOutput = (inputs[0] * zeroCoeff) + temp;
   inputs[0] = temp;
   return lastOutput;
}


