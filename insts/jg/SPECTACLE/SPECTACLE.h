#include "SPECTACLE_BASE.h"

class SPECTACLE : public SPECTACLE_BASE {

   float    *eqtable, *deltimetable, *feedbacktable;
   DLineN   *phase_delay[MAXFFTLEN / 2], *mag_delay[MAXFFTLEN / 2];

public:
   SPECTACLE();
   virtual ~SPECTACLE();

protected:
   virtual const char *instname() { return "SPECTACLE"; }
   virtual int pre_init(float *, int);
   virtual int post_init(float *, int);
   virtual void dump_anal_channels();
   virtual void modify_analysis();
};

