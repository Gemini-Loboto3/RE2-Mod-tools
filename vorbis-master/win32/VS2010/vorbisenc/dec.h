#pragma once
typedef unsigned long DWORD;
typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;
typedef signed   int   s32;
typedef signed   short s16;
typedef signed   char  s8;

typedef struct tagWavChunk
{
	DWORD magic;
	DWORD size;
} WAV_CHUNK;

typedef struct  WAV_HEADER {
	unsigned long       RIFF;           // RIFF Header      Magic header
	unsigned long       ChunkSize;      // RIFF Chunk Size  
	unsigned long       WAVE;           // WAVE Header      
	unsigned long       fmt;            // FMT header       
	unsigned long       Subchunk1Size;  // Size of the fmt chunk                                
	unsigned short      AudioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
	unsigned short      NumOfChan;      // Number of channels 1=Mono 2=Sterio                   
	unsigned long       SamplesPerSec;  // Sampling Frequency in Hz                             
	unsigned long       bytesPerSec;    // bytes per second 
	unsigned short      blockAlign;     // 2=16-bit mono, 4=16-bit stereo 
	unsigned short      bitsPerSample;  // Number of bits per sample      
} wav_hdr;

typedef struct tagRiffHeader
{
	DWORD magic;
	DWORD size;
	DWORD type;
} RIFF_HEADER;

typedef struct tagWavfmt
{
	unsigned long       fmt;			// FMT header       
	unsigned long       Subchunk1Size;	// Size of the fmt chunk                                
	unsigned short      AudioFormat;	// Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM 
	unsigned short      Channels;		// Number of channels 1=Mono 2=Sterio                   
	unsigned long       SampleRate;		// Sampling Frequency in Hz                             
	unsigned long       bytesPerSec;	// SamplingRate * BlockAlign
	unsigned short      blockAlign;		// Channels * BitsPerSample / 8
	unsigned short      bitsPerSample;	// 8 or 16
} WAVFMT;

typedef enum PCM_Type
{
	PT_SIGNED16,
	PT_UNSIGNED16,
	PT_SIGNED8,
	PT_UNSIGNED8,
	PT_FLOAT,
	PT_DOUBLE,
	PT_UNSUPPORTED
} PCM_Type;

typedef struct tagWave
{
	unsigned char		*data;
	DWORD				size;			// data chunk size
	DWORD				NumOfChan;      // Number of channels 1=Mono 2=Sterio                   
	DWORD				SamplesPerSec;  // Sampling Frequency in Hz
	// internal variables
	u32					samples;		// number of samples the WAVE has
	u32					consumed;		// number of samples currently retrieved
	PCM_Type			type;			// format to convert to when consuming
} WAVE;

#define FOURCC(a,b,c,d)		(a | (b<<8) | (c<<16) | (d<<24))

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif
	void convert(FILE *in, FILE *out, float quality);

	void open_wave(FILE *in, WAVE *wave);
	void close_wave(WAVE *wave);
	int consume_samples(WAVE *wave, int samples, s16 *dst);
#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif
