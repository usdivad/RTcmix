#include "SPECTACLE_BASE.h"

class TVSPECTACLE : public SPECTACLE_BASE {

private:
   long     maxdelsamps;
   float    eq_curve_weight, deltime_curve_weight, feedback_curve_weight;
   float    *eqtableA, *deltimetableA, *feedbacktableA;
   float    *eqtableB, *deltimetableB, *feedbacktableB;
   float    *eqcurve, *deltimecurve, *feedbackcurve;
   float    eqcurvetabs[2], deltimecurvetabs[2], feedbackcurvetabs[2];
   DLineN   *phase_delay[MAXFFTLEN / 2], *mag_delay[MAXFFTLEN / 2];

public:
   TVSPECTACLE();
   virtual ~TVSPECTACLE();

protected:
   virtual const char *instname() { return "TVSPECTACLE"; }
   virtual int pre_init(float *, int);
   virtual int post_init(float *, int);
   virtual void dump_anal_channels();
   virtual void modify_analysis();
};

