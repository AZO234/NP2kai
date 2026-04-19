/**
 * @file	asiosdk.h
 */

#pragma once

/**
 * Boolean values
 */
typedef long ASIOBool;

/**
 * Boolean values
 */
enum
{
	ASIOFalse = 0,
	ASIOTrue = 1
};

/**
 * Error code
 */
typedef long ASIOError;

/**
 * Error code
 */
enum
{
	ASE_OK = 0,						//!< This value will be returned whenever the call succeeded
	ASE_SUCCESS = 0x3f4847a0,		//!< unique success return value for ASIOFuture calls
	ASE_NotPresent = -1000,			//!< hardware input or output is not present or available
	ASE_HWMalfunction,				//!< hardware is malfunctioning (can be returned by any ASIO function)
	ASE_InvalidParameter,			//!< input parameter invalid
	ASE_InvalidMode,				//!< hardware is in a bad mode or used in a bad mode
	ASE_SPNotAdvancing,				//!< hardware is not running when sample position is inquired
	ASE_NoClock, 					//!< sample clock or rate cannot be determined or is not present
	ASE_NoMemory					//!< not enough memory for completing the request
};

/**
 * Sample rate
 */
typedef double ASIOSampleRate;

struct ASIOClockSource;
struct ASIOSamples;
struct ASIOTimeStamp;

/**
 * Sample Types
 */
typedef long ASIOSampleType;

/**
 * Sample Types
 */
enum
{
	ASIOSTInt16MSB		= 0,		//!< 16 bit data word
	ASIOSTInt24MSB		= 1,		//!< This is the packed 24 bit format. used for 20 bits as well
	ASIOSTInt32MSB		= 2,
	ASIOSTFloat32MSB	= 3,		// IEEE 754 32 bit float
	ASIOSTFloat64MSB	= 4,		// IEEE 754 64 bit double float

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can be more easily used with these
	ASIOSTInt32MSB16	= 8,		// 32 bit data with 16 bit alignment
	ASIOSTInt32MSB18	= 9,		// 32 bit data with 18 bit alignment
	ASIOSTInt32MSB20	= 10,		// 32 bit data with 20 bit alignment
	ASIOSTInt32MSB24	= 11,		// 32 bit data with 24 bit alignment
	
	ASIOSTInt16LSB		= 16,
	ASIOSTInt24LSB		= 17,		// used for 20 bits as well
	ASIOSTInt32LSB		= 18,
	ASIOSTFloat32LSB	= 19,		// IEEE 754 32 bit float, as found on Intel x86 architecture
	ASIOSTFloat64LSB	= 20, 		// IEEE 754 64 bit double float, as found on Intel x86 architecture

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can more easily used with these
	ASIOSTInt32LSB16	= 24,		// 32 bit data with 18 bit alignment
	ASIOSTInt32LSB18	= 25,		// 32 bit data with 18 bit alignment
	ASIOSTInt32LSB20	= 26,		// 32 bit data with 20 bit alignment
	ASIOSTInt32LSB24	= 27,		// 32 bit data with 24 bit alignment

	//	ASIO DSD format.
	ASIOSTDSDInt8LSB1	= 32,		// DSD 1 bit data, 8 samples per byte. First sample in Least significant bit.
	ASIOSTDSDInt8MSB1	= 33,		// DSD 1 bit data, 8 samples per byte. First sample in Most significant bit.
	ASIOSTDSDInt8NER8	= 40,		// DSD 8 bit data, 1 sample per byte. No Endianness required.

	ASIOSTLastEntry
};

/**
 * @brief Channel info
 */
struct ASIOChannelInfo
{
	long channel;					//!< on input, channel index
	ASIOBool isInput;				//!< on input
	ASIOBool isActive;				//!< on exit
	long channelGroup;				//!< dto
	ASIOSampleType type;			//!< dto
	char name[32];					//!< dto
};

/**
 * @brief Buffer info
 */
struct ASIOBufferInfo
{
	ASIOBool isInput;				//!< on input: ASIOTrue: input, else output
	long channelNum;				//!< on input: channel index
	void* buffers[2];				//!< on output: double buffer addresses
};

struct ASIOTime;

/**
 * @brief Callback
 */
struct ASIOCallbacks
{
	void (*bufferSwitch)(long doubleBufferIndex, ASIOBool directProcess);
	void (*sampleRateDidChange)(ASIOSampleRate sRate);
	long (*asioMessage)(long selector, long value, void* message, double* opt);
	ASIOTime* (*bufferSwitchTimeInfo)(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
};

/**
 * @brief ASIO インタフェイス
 */
interface IASIO : public IUnknown
{
	virtual ASIOBool init(void* sysHandle) = 0;
	virtual void getDriverName(char* name) = 0;	
	virtual long getDriverVersion() = 0;
	virtual void getErrorMessage(char* string) = 0;	
	virtual ASIOError start() = 0;
	virtual ASIOError stop() = 0;
	virtual ASIOError getChannels(long* numInputChannels, long* numOutputChannels) = 0;
	virtual ASIOError getLatencies(long* inputLatency, long* outputLatency) = 0;
	virtual ASIOError getBufferSize(long* minSize, long* maxSize, long* preferredSize, long* granularity) = 0;
	virtual ASIOError canSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getSampleRate(ASIOSampleRate* sampleRate) = 0;
	virtual ASIOError setSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getClockSources(ASIOClockSource* clocks, long* numSources) = 0;
	virtual ASIOError setClockSource(long reference) = 0;
	virtual ASIOError getSamplePosition(ASIOSamples* sPos, ASIOTimeStamp* tStamp) = 0;
	virtual ASIOError getChannelInfo(ASIOChannelInfo* info) = 0;
	virtual ASIOError createBuffers(ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks) = 0;
	virtual ASIOError disposeBuffers() = 0;
	virtual ASIOError controlPanel() = 0;
	virtual ASIOError future(long selector, void* opt) = 0;
	virtual ASIOError outputReady() = 0;
};
