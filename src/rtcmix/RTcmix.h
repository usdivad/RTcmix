#ifndef _RTCMIX_H_
#define _RTCMIX_H_

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

#include <rtdefs.h>
#include <rt_types.h>
#include <bus.h>
#include "Locked.h"

extern "C" void set_SR(float);

class Instrument;
class PFieldSet;
class RTQueue;
class heap;
class AudioDevice;
class Arg;
class TaskManager;

struct _handle;
typedef struct _handle *Handle;
struct rt_item;
struct BusQueue;
struct CheckNode;
struct BusSlot;
struct _func;
struct FunctionEntry;
struct InputState;	// part of Instrument class

enum RTstatus {
	RT_GOOD = 0, RT_SHUTDOWN = 1, RT_PANIC = 2, RT_ERROR = 3
};

enum RTInputError {
	RT_NO_ERROR = 0,
	RT_NO_INPUT_SRC = -1,			// no input file open
	RT_ILLEGAL_DEV_OFFSET = -2,		// nonzero offset for audio input device
	RT_INPUT_EOF = -3,				// start offset beyond end of input file (nonfatal)
	RT_INPUT_CHANS_MISMATCH = -4	// specified input chan count != input file count
};

/* definition of input file desc struct used by rtinput */
struct InputDesc : public Lockable {
	char     *filename;         /* allocated by rtinput() */
	int      fd;                /* file descriptor, or NO_FD, or AUDIO_DEVICE */
	int      refcount;
	short    is_audio_dev;      /* true if input from audio dev, not from file */
	short    header_type;       /* e.g., AIFF_sound_file (in sndlib.h) */
	short    data_format;       /* e.g., snd_16_linear (in sndlib.h) */
	short    is_float_format;   /* true if data format is 32-bit float */
	int      data_location;     /* offset of sound data start in file */
	long     endbyte;           /* index of byte following last sample */
	float    srate;
	short    chans;
	double   dur;
};

class RTcmix {
public:
	RTcmix();				// 44.1k/stereo default
	RTcmix(float, int);		// set SR and NCHANS
	RTcmix(float, int, int, const char* opt1=0, const char *opt2=0, const char *opt3=0);	// set SR, NCHANS, BUFSIZE, up to 3 options
	virtual ~RTcmix();

	
	Instrument* cmd(const char*, int, double, ...); // for numeric params
	Instrument* cmd(const char*, int, const char*, ...); // for string params
	Instrument* cmd(const char*, const PFieldSet &); // for PFieldSet

	double cmd(const char*); // for commands with no params
	double cmdval(const char*, int, double, ...); // value return (numeric params)
	double cmdval(const char*, int, const char*, ...); // value return (string params)

	void printOn();
	void printOff();
	void panic();
	void close();
	virtual void run();	

	// New public API

	static bool interactive() { return rtInteractive; }
	static int bufsamps() { return RTBUFSAMPS; }
	static float sr() { return SR; }
	static int chans() { return NCHANS; }
	static long getElapsedFrames() { return elapsed + bufsamps(); }
	static bool outputOpen() { return rtfileit != -1; }
	static bool rtsetparams_was_called() { return rtsetparams_called; }
	
	static int registerFunction(const char *funcName, const char *dsoPath);
	static int dispatch(const char *func_label, const Arg arglist[], 
						const int nargs, Arg *retval);
	static void addfunc(const char *func_label,
					   double (*func_ptr_legacy)(float*, int, double*),
                       double (*func_ptr_number)(const Arg[], int),
                       char * (*func_ptr_string)(const Arg[], int),
                       Handle (*func_ptr_handle)(const Arg[], int),
                       int    return_type,
                       int    legacy);
	static int addrtInst(rt_item *);
	
	// These are called by Instrument class -- can it be done using friends?
	// Config
	static BusSlot *get_bus_config(const char *inst_name);
	static bool isInputAudioDevice(int fdIndex) { return inputFileTable[fdIndex].is_audio_dev; }
    static const char *getInputPath(int fdIndex) { return inputFileTable[fdIndex].filename; }
	// Input
	static int attachInput(float inputSkip, InputState *instInput);
	static void readFromAuxBus(BufPtr dest, int dest_chans, int dest_frms, const short src_chan_list[], short src_chans, int output_offset);
	static void readFromAudioDevice(BufPtr dest, int dest_chans, int dest_frms, const short src_chan_list[], short src_chans, int output_offset);
	static off_t seekInputFile(int fdIndex, int frames, int chans, int whence);
	static void readFromInputFile(BufPtr dest, int dest_chans, int dest_frms, const short src_chan_list[], short src_chans, int fdIndex, off_t *outFileOffset);
	static void rtgetsamps(AudioDevice *inputDevice);
	// Output	   
	static void addToBus(BusType type, int bus, BufPtr buf, int offset, int endfr, int chans);
	static void releaseInput(int fdIndex);

	// These are functions called from the parser via pointers, and are
	// registered via rt_ug_intro().
	static double rtsetparams(float*, int, double *);
	static double rtinput(float*, int, double *);
	static double rtoutput(float*, int, double *);
	static double set_option(float *, int, double *);
	static double bus_config(float*, int, double *);
	// Minc information functions as methods
	static double input_chans(float *, int);
	static double input_dur(float *, int);
	static double input_sr(float *, int);
	static double input_peak(float *, int);
	static double left_peak(float *, int);
	static double right_peak(float *, int);

protected:
	RTcmix(bool dummy);				// Called by RTcmixMain class

	// Initialization methods.
	void init(float, int, int, const char*, const char*, const char*);	// called by all constructors
	static void init_globals(bool fromMain, const char *defaultDSOPath);

	// Cleanup methods
	static void free_globals();
	static void free_bus_config();
	
	// Audio loop methods
	
	int runMainLoop();
	int waitForMainLoop();

	// These were standalone but are now static methods
	static double checkInsts(const char *instname, const Arg arglist[], const int nargs, Arg *retval);
	static int checkfunc(const char *funcname, const Arg arglist[], const int nargs, Arg *retval);
	static int findAndLoadFunction(const char *funcname);
	static void freefuncs();
	static long getElapsed() { return elapsed; }

protected:
	/* Note: these 3 vars also extern in rtdefs.h, for use by insts */
	static int 		NCHANS;
	static int 		RTBUFSAMPS;
	static float 	SR;
	
	static int		rtInteractive;
	static int		rtsetparams_called;
	static int		audioLoopStarted;
	static int		audio_config;

	static AudioDevice *audioDevice;

	static RTstatus	run_status;

	static pthread_mutex_t audio_config_lock;

private:
	// Buffer alloc routines.
	static int allocate_audioin_buffer(short chan, int len);
	static int allocate_aux_buffer(short chan, int len);
	static int allocate_out_buffer(short chan, int len);

	// Cleanup routine.
	static void free_buffers();

	static int registerDSOs(const char *dsoPaths);

	// Internal audio loop methods (called by runMainLoop())
	
	static bool inTraverse(AudioDevice *, void *);
	static int rtsendsamps(AudioDevice *);
	static int rtwritesamps(AudioDevice *);
	static void limiter(BUFTYPE peaks[], long peaklocs[]);
	static int rtsendzeros(AudioDevice *device, int);
	static void rtreportstats(AudioDevice *);
	static bool doneTraverse(AudioDevice *, void *);
	
	static float get_peak(float, float, int);
	static int parse_rtoutput_args(int nargs, double pp[]);
	
	// Buffer methods
	static void init_buf_ptrs();
	static void clear_aux_buffers();
	static void clear_output_buffers();
	
	friend void set_SR(float);	// hack to allow C code to initialize SR

	static int		audioNCHANS;

	/* ---------------------------------------------------------------------- */

	static int		output_data_format;
	static int		output_header_type;
	static int		normalize_output_floats;
	static int		is_float_format;
	static char *	rtoutsfname;


	/* used in intraverse.C, rtsendsamps.c */
	static unsigned long bufStartSamp;
	static long		elapsed;

	// DT:  main heap structure used to queue instruments
	static heap *rtHeap;
	static RTQueue *rtQueue;
	static rt_item *rt_list;

	static pthread_mutex_t pfieldLock;
	static pthread_mutex_t aux_to_aux_lock;
	static pthread_mutex_t to_aux_lock;
	static pthread_mutex_t to_out_lock;
	static pthread_mutex_t inst_bus_config_lock;
	static pthread_mutex_t bus_in_config_lock;
	static pthread_mutex_t has_child_lock;
	static pthread_mutex_t has_parent_lock;
	static pthread_mutex_t aux_in_use_lock;
	static pthread_mutex_t aux_out_in_use_lock;
	static pthread_mutex_t out_in_use_lock;
	static pthread_mutex_t revplay_lock;
	static pthread_mutex_t bus_slot_lock;

	static int		rtrecord;
	static int		rtfileit;		// 1 if rtoutput() succeeded
	static int		rtoutfile;

	static InputDesc	*inputFileTable;
	static int max_input_fds;

	static BufPtr	audioin_buffer[];    /* input from ADC, not file */
	static BufPtr	aux_buffer[];
	static BufPtr	out_buffer[];
#ifdef MULTI_THREAD
	static TaskManager *taskManager;
	static pthread_mutex_t aux_buffer_lock;
	static pthread_mutex_t out_buffer_lock;
#endif
	
	static short AuxToAuxPlayList[]; /* The playback order for AUX buses */
	static short ToOutPlayList[]; /* The playback order for AUX buses */
	static short ToAuxPlayList[]; /* The playback order for AUX buses */


	// Bus configuration methods
	static void print_parents();
	static void print_children();
	static void print_play_order();
	static ErrCode parse_bus_chan(char*, int*, int*); /* Input processing */
	static ErrCode check_bus_inst_config(BusSlot*, Bool);  /* Graph parsing, insertion */
	static ErrCode print_inst_bus_config();
	static ErrCode insert_bus_slot(char*, BusSlot*);
	static void bf_traverse(int bus, Bool visit);
	static void create_play_order();

	// Bus configuration variables

	/* List of lists of BusSlots used by Insts to get their bus_config setup */
	static BusQueue *Inst_Bus_Config;

	/* Flag to tell us if we've gotten any configs */
	/* Used to initialize Bus_In_Config inside check_bus_inst_config */
	static Locked<Bool> Bus_Config_Status;

	/* Bus graph, parsed by check_bus_inst_config */
	/* Allows loop checking ... and buffer playback order? */
	static CheckNode *Bus_In_Config[MAXBUS];

	/* Global private arrays */
	static Bool HasChild[MAXBUS];
	static Bool HasParent[MAXBUS];
	static Bool AuxInUse[MAXBUS];
	static Bool AuxOutInUse[MAXBUS];
	static Bool OutInUse[MAXBUS];
	static short RevPlay[MAXBUS];
	
	// END of bus config
	
	// Function registry
	static FunctionEntry *_functionRegistry;

	// Function table variables
	static struct _func *	_func_list;
	// End of function table

};

// handy utility function...
/* RTtimeit takes a floating point number of seconds (interval) and a pointer
   to a void-returning function and sets up a timer to call that function
   every interval seconds.  Setting interval to 0.0 should disable the
   timer */
void RTtimeit(float interval, sig_t func);

#endif	// _RTCMIX_H_
