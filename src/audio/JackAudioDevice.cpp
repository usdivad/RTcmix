// JackAudioDevice.cpp

#if defined(JACK)

#include "JackAudioDevice.h"
#include <jack/jack.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 1

#if DEBUG > 1
#define PRINT0 if (1) printf
#define PRINT1 if (1) printf
#elif DEBUG > 0
#define PRINT0 if (1) printf
#define PRINT1 if (0) printf
#else
#define PRINT0 if (0) printf
#define PRINT1 if (0) printf
#endif


// Our methods are called in this order during create_audio_devices()...
//
// recognize()
// create()
// doOpen()
// doSetFormat()
// doSetQueueSize()
//
// Then these are called...
//
// doStart()          // during RTcmix::runMainLoop()
// doSendFrames()     // during RTcmix::rtsendsamps()
// doGetFrames()      // during RTcmix::rtgetsamps()
// doStop()           // during, errr, idunno

// Here is a partial list of base class helper methods you may use to check
// the state of the system.  Not all of these have valid values at all times --
// some return state that you set via the init/open sequence.  Most should be
// self-explanatory:

// bool		isOpen() const;
// bool		isRunning() const;				-- has start() been called
// bool		isPaused() const;
// int		getFrameFormat() const;			-- MUS_BSHORT, etc.
// int		getDeviceFormat() const;
// bool		isFrameFmtNormalized() const;
// bool		isDeviceFmtNormalized() const;
// bool		isFrameInterleaved() const;
// bool		isDeviceInterleaved() const;
// int		getFrameChannels() const;
// int		getDeviceChannels() const;
// double	getSamplingRate() const;
// long		getFrameCount() const;			-- number of frames rec'd or played


// This struct allows us to hide all Jack implementation details within
// this source file.

struct JackAudioDevice::Impl {
	// Put all class-specific state in here.  You can also add a constructor
	// to make sure all state is initialized, or do it by hand in the main
	// class constructor directly below.  Ditto with destructor.
	Impl() : client(NULL), numInPorts(0), numOutPorts(0), inPorts(NULL),
		outPorts(NULL), inBuf(NULL), outBuf(NULL), frameCount(0),
		jackOpen(false), jackActivated(false) {}
	~Impl();
	jack_client_t *client;
	int numInPorts;
	int numOutPorts;
	jack_port_t **inPorts;
	jack_port_t **outPorts;
	jack_default_audio_sample_t **inBuf;
	jack_default_audio_sample_t **outBuf;
	int frameCount;
	bool jackOpen;
	bool jackActivated;

	static int runProcess(jack_nframes_t nframes, void *object);
	static void shutdown(void *object);
};

JackAudioDevice::Impl::~Impl()
{
#if 0 // wait, maybe the ports are owned by JACK?
	use jack_port_unregister(), or just forget it?
	for (int i = 0; i < numInPorts; i++)
		delete inPorts[i];
#endif
	delete [] inPorts;
	delete [] outPorts;
	delete [] inBuf;
	delete [] outBuf;
}

int JackAudioDevice::Impl::runProcess(jack_nframes_t nframes, void *object)
{
	PRINT1("JackAudioDevice::runProcess(nframes=%d)\n", nframes);
	JackAudioDevice *device = (JackAudioDevice *) object;
	if (!device->isRunning())	// callback runs before doStart called
		return 0;
	JackAudioDevice::Impl *impl = device->_impl;
	const int inchans = impl->numInPorts;
	const int outchans = impl->numOutPorts;
	jack_default_audio_sample_t **in = impl->inBuf;
	jack_default_audio_sample_t **out = impl->outBuf;

	// get non-interleaved buffer pointers from JACK
	for (int i = 0; i < inchans; i++)
		in[i] = (jack_default_audio_sample_t *)
		                        jack_port_get_buffer(impl->inPorts[i], nframes);
// FIXME: supposedly the out buffer ptrs can be cached btw. calls to blocksize
// callback -- see docs for jack_port_get_buffer
	for (int i = 0; i < outchans; i++)
		out[i] = (jack_default_audio_sample_t *)
		                        jack_port_get_buffer(impl->outPorts[i], nframes);

	// copy buffers
// we'll try to do this in doGetFrames/doSendFrames


	// process sound

#if 0
	bool keepGoing = true;
	while (framesAvail < framesToRead && keepGoing) {
		keepGoing = device->runCallback();
	}
#else
	bool keepGoing = device->runCallback();
	if (!keepGoing) {
		device->stopCallback();
		device->close();
	}
#endif

	impl->frameCount += nframes;

	return 0;
}

// Called when JACK server shuts down.
void JackAudioDevice::Impl::shutdown(void *object)
{
	PRINT0("JackAudioDevice::shutdown()\n");
	JackAudioDevice *device = (JackAudioDevice *) object;
	device->stopCallback();
	device->close();
	// FIXME: make RTcmix quit gracefully
}

JackAudioDevice::JackAudioDevice() : _impl(new Impl)
{
}

JackAudioDevice::~JackAudioDevice()
{ 
	close();
	delete _impl;
}

// doOpen() is called by AudioDevice::open() to do the class-specific opening
// of the audio port, HW, device, etc.  and set up any remaining Impl state.
//
// The assumption is that the open of the HW will return a integer file
// descriptor that we can wait on.  Before exiting this method, call
// setDevice() and hand it that descriptor.  It is used by waitForDevice().
//
// 'mode' has a direction portion and a bit to indicate if the device is being
// run in passive mode (does not spawn its own thread to handle I/O).
// You are guaranteed that doOpen() will NOT be called if you are already open.

int JackAudioDevice::doOpen(int mode)
{
	// Initialize any Impl state if not done in Impl::Impl().

// FIXME: RTcmix is not always the program name
// also, consider using jack_client_open, as in simple_client example
	_impl->client = jack_client_new("RTcmix");
	if (_impl->client == 0)
		return error("JACK server not running?");
	_impl->jackOpen = true;
	if (jack_set_process_callback(_impl->client, _impl->runProcess,
			(void *) this) != 0)
		return error("Could not register JACK process callback.");
	jack_on_shutdown(_impl->client, _impl->shutdown, (void *) this);

	return 0;
}

// doClose() is called by AudioDevice::close() to do the class-specific closing
// of the audio port, HW, device, etc.  You are guaranteed that doClose() will
// NOT be called if you are already closed.

int JackAudioDevice::doClose()
{
	PRINT0("JackAudioDevice::doClose()\n");
	if (isOpen()) {
		if (_impl->jackOpen) {
			// must clear this right away to prevent race condition
			_impl->jackOpen = false;
			PRINT0("...calling jack_client_close(client=%p)\n", _impl->client);
			if (jack_client_close(_impl->client) != 0)
				return error("Error closing JACK.");
		}
// FIXME: not the right place (see other stopCallback invocations in this file),
// but the only way to handle interrupt correctly?
		stopCallback();
	}
	_impl->frameCount = 0;
	return 0;
}

// Connect JACK ports (must be done after activating JACK)

int JackAudioDevice::connectPorts()
{
	// By default we connect to hardware in/out ports that make sense.
	// They can be overridden by port names given in set_option calls.

	if (1) {
		if (isRecording()) {
			const char **ports = jack_get_ports(_impl->client, NULL, NULL,
			                                JackPortIsPhysical | JackPortIsOutput);
			if (ports == NULL)
				return error("no physical capture ports");

			const int numports = getFrameChannels();
			for (int i = 0; i < numports; i++) {
				const char *srcport = ports[i];
				if (srcport == NULL)
					break;
				const char *destport = jack_port_name(_impl->inPorts[i]);
				if (jack_connect(_impl->client, srcport, destport) != 0)
					return error("cannot connect input ports");
			}
			free(ports);
		}
		if (isPlaying()) {
			const char **ports = jack_get_ports(_impl->client, NULL, NULL,
			                                JackPortIsPhysical | JackPortIsInput);
			if (ports == NULL)
				return error("no physical playback ports");

			const int numports = getFrameChannels();
			for (int i = 0; i < numports; i++) {
				const char *srcport = jack_port_name(_impl->outPorts[i]);
				const char *destport = ports[i];
				if (destport == NULL)
					break;
				if (jack_connect(_impl->client, srcport, destport) != 0)
					return error("cannot connect output ports");
			}
			free(ports);
		}
	}

	return 0;
}

int JackAudioDevice::doStart()
{
	PRINT0("JackAudioDevice::doStart()\n");

	// NB: This can't be done before coordinating buffer size.
	if (jack_activate(_impl->client) != 0)
		return error("Error activating JACK client.");
	_impl->jackActivated = true;

	return connectPorts();
}

int JackAudioDevice::doPause(bool)
{
	return error("Not implemented");
}

int JackAudioDevice::doStop()
{
	PRINT0("JackAudioDevice::doStop()\n");

#if 0
	if (_impl->jackActivated) {
		// must clear this right away to prevent race condition
		_impl->jackActivated = false;
		if (jack_deactivate(_impl->client) != 0)
			return error("Error deactivating JACK client.");
	}
#endif

	return 0;
}

// doSetFormat() is called by AudioDevice::setFormat() and by
// AudioDevice::open().  Here is where you configure your HW, setting it to the
// format which will best handle the format passed in.  Note that it is NOT
// necessary for the HW to match the input format except in sampling rate;  The
// base class can handle most format conversions.
//
// 'sampfmt' is the format of the data passed to AudioDevice::getFrames() or
//	AudioDevice::sendFrames(), and has three attributes:
//	1) The actual type of the format, retrieved via MUS_GET_FORMAT(sampfmt)
//	2) The interleave (true or false) retrieved via MUS_GET_INTERLEAVE(sampfmt)
//	3) Whether the samples (when float) are normalized, retrieved via
//		MUS_GET_NORMALIZED(sampfmt)
//
// At the end of this method, you must call setDeviceParams() to notify the
// base class what format *you* need the audio data to be in.

int JackAudioDevice::doSetFormat(int sampfmt, int chans, double srate)
{
	// Insure that RTcmix sampling rate matches JACK rate.
	jack_nframes_t jsrate = jack_get_sample_rate(_impl->client);
	if (jsrate != (jack_nframes_t) srate) {
		char msg[256];
		snprintf(msg, 256, "RTcmix sampling rate (set in rtsetparams) must match "
					"JACK sampling rate (currently %d)", jsrate);
		return error(msg);
// FIXME: can't we just change RTcmix::SR instead, and also notify user?
	}

// FIXME:
// we just want to handle non-interleaved, normalized, 32bit floats in host byte order
// what is the relation between this and <sampfmt> passed to us?
//	int deviceFormat = MUS_GET_FORMAT(sampfmt);

//	int deviceFormat = MUS_NON_INTERLEAVED | MUS_NORMALIZED;
	int deviceFormat = MUS_INTERLEAVED | MUS_NORMALIZED;
#if MUS_LITTLE_ENDIAN
	deviceFormat |= MUS_LFLOAT;
#else
	deviceFormat |= MUS_BFLOAT;
#endif

	setDeviceParams(deviceFormat, chans, srate);

	return 0;
}

// doSetQueueSize() is called by AudioDevice::setQueueSize() to allow
// HW-specific configuration of internal audio queue sizes.  The values handed
// in via address represent the size **in frames** of the buffers that will be
// handed to doGetFrames() and/or doSendFrames(), and the number of such
// buffers the application would like to have queued up for robustness.  The
// actual frame count as determined by your HW *must* be reported back to the
// caller via 'pWriteSize'.  If you cannot match *pCount, just do the best you
// can, but do not fail if you cannot match it.

int JackAudioDevice::doSetQueueSize(int *pWriteSize, int *pCount)
{
	jack_nframes_t jackBufSize = jack_get_buffer_size(_impl->client);
#if 1
// FIXME: I doubt this is right...
	jack_nframes_t rtcmixBufSize = *pWriteSize * *pCount;
	if (rtcmixBufSize != jackBufSize) {
		*pWriteSize = jackBufSize / *pCount;
		// notify user
	}
#else
	if ((jack_nframes_t) *pWriteSize != jackBufSize) {
		*pWriteSize = jackBufSize;
		// notify user?
	}
	if (*pCount != 1) {
		*pCount = 1;
		// notify user?
	}
#endif

	// Create JACK ports, one per channel, and audio buf pointer arrays.

	if (isRecording()) {
		// FIXME: we shouldn't assume that this is the same as output chans;
		// whatever happened to audioNCHANS?
		const int numports = getFrameChannels();
		_impl->inPorts = new jack_port_t * [numports];
		_impl->inBuf = new jack_default_audio_sample_t * [numports];
		char shortname[32];
		for (int i = 0; i < numports; i++) {
			sprintf(shortname, "in_%d", i + 1);
			jack_port_t *port = jack_port_register(_impl->client, shortname,
			                                       JACK_DEFAULT_AUDIO_TYPE,
			                                       JackPortIsInput, 0);
			if (port == NULL)
				return error("no more JACK ports available");
			_impl->inPorts[i] = port;
			_impl->inBuf[i] = NULL;    // set in runProcess
		}
		_impl->numInPorts = numports;
	}
	if (isPlaying()) {
		const int numports = getFrameChannels();
		_impl->outPorts = new jack_port_t * [numports];
		_impl->outBuf = new jack_default_audio_sample_t * [numports];
		char shortname[32];
		for (int i = 0; i < numports; i++) {
			sprintf(shortname, "out_%d", i + 1);
			jack_port_t *port = jack_port_register(_impl->client, shortname,
			                                       JACK_DEFAULT_AUDIO_TYPE,
			                                       JackPortIsOutput, 0);
			if (port == NULL)
				return error("no more JACK ports available");
			_impl->outPorts[i] = port;
			_impl->outBuf[i] = NULL;   // set in runProcess
		}
		_impl->numOutPorts = numports;
	}

	return 0;
}

// doGetFrames() is called by AudioDevice::getFrames() during record.
// The format of 'frameBuffer' will be the format **YOU** specified via
// setDeviceParams() above.  It will be converted into the 'frame format'
// by a base class.  Here is where you fill frameBuffer from your audio HW.

int JackAudioDevice::doGetFrames(void *frameBuffer, int frameCount)
{
	const int chans = getFrameChannels();
	float *dest = (float *) frameBuffer;

	for (int i = 0; i < frameCount; i++) {
		for (int ch = 0; ch < chans; ch++)
			dest[ch] = (float) _impl->inBuf[ch][i];
		dest += chans;
	}

	return frameCount;
}

// doSendFrames() is called by AudioDevice::sendFrames() during playback.
// The format of 'frameBuffer' will be the format **YOU** specified via
// setDeviceParams() above.  It was converted from the 'frame format'
// by a base class.   Here is where you hand the audio in frameBuffer to you
// HW.

int JackAudioDevice::doSendFrames(void *frameBuffer, int frameCount)
{
	const int chans = getFrameChannels();
	float *src = (float *) frameBuffer;

	for (int i = 0; i < frameCount; i++) {
		for (int ch = 0; ch < chans; ch++)
			_impl->outBuf[ch][i] = (jack_default_audio_sample_t) src[ch];
		src += chans;
	}

	return frameCount;
}

int JackAudioDevice::doGetFrameCount() const
{
	return _impl->frameCount;
}

// Return true if the passed in device descriptor matches one that this device
// can understand.

bool JackAudioDevice::recognize(const char *desc)
{
	return (strncmp(desc, "JACK", 4) == 0);
}

// If your audio device(s) needs a string descriptor, it will come in via
// 'inputDesc' and/or 'outputDesc', allowing you to specify different HW for
// record and play.

AudioDevice *JackAudioDevice::create(const char *inputDesc,
	const char *outputDesc, int mode)
{
	return new JackAudioDevice;
}

#endif	// JACK
