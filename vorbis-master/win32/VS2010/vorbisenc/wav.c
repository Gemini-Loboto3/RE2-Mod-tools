#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dec.h"

#define USE_LINEAR	0

// read ahead, don't update position
void peek_chunk(unsigned char *in, long *pos, WAV_CHUNK *chunk)
{
	memcpy(chunk, &in[*pos], sizeof(*chunk));
}

// consume a whole chunk and update position
void consume_chunk(unsigned char *in, long *pos, void *dst, int size)
{
	WAV_CHUNK chunk;

	// read chunk size
	memcpy(&chunk, &in[*pos], sizeof(chunk));
	// copy data to our chunk
	memcpy(dst, &in[*pos], size);

	// get to next chunk
	*pos += chunk.size + sizeof(chunk);
}

//static int handle_overflow(int *sample)
//{
//	if (*sample > 32767) {
//		*sample = 32767;
//		return 1;
//	}
//	else if (*sample < -32768) {
//		*sample = -32768;
//		return -1;
//	}
//	return 0;
//}
//
//static __inline short lerp(short a, short b, double lerp)
//{
//	return (short)((double)a * lerp) + ((double)b * (1. - lerp));
//}

//size_t interpolate(short *in, short **out, size_t size, int channels, int rate)
//{
//	double const step = 37800. / 44100.;
//	*out = (short*)malloc(size * 2);	// allocate a buffer large enough
//
//	short *pout = *out;
//	for (double i = 0., si = (double)size / 2.; i < si; i += step)
//		*pout++ = lerp(in[(int)i], in[(int)i + 1], step);
//
//	return (size_t)((unsigned)pout - (unsigned)*out);
//}

void open_wave(FILE *in, WAVE *wave)
{
	memset(wave, 0, sizeof(*wave));

	RIFF_HEADER riff;
	fread(&riff, sizeof(riff), 1, in);

	if (riff.magic != FOURCC('R', 'I', 'F', 'F') &&
		riff.type != FOURCC('W', 'A', 'V', 'E'))
		return;

	// cache all wave data
	long pos = 0;
	unsigned char *buffer = (unsigned char*)malloc(riff.size - 4);
	fread(buffer, riff.size - 4, 1, in);

	WAVFMT fmt;
	while (pos + (long)sizeof(WAV_CHUNK) < (long)riff.size - 4)
	{
		WAV_CHUNK chunk;
		peek_chunk(buffer, &pos, &chunk);
		switch (chunk.magic)
		{
		case FOURCC('f','m','t',' '):
			consume_chunk(buffer, &pos, &fmt, sizeof(fmt));
			wave->NumOfChan = fmt.Channels;
			wave->SamplesPerSec = fmt.SampleRate;
			switch (fmt.AudioFormat)
			{
			case 1:	// PCM
				if (fmt.bitsPerSample == 8)
					wave->type = PT_SIGNED8;
				else if (fmt.bitsPerSample == 16)
					wave->type = PT_SIGNED16;
				// not compilant, throw error
				else wave->type = PT_UNSUPPORTED;
				break;
			case 3:	// IEEE
				// float
				if (fmt.bitsPerSample == 32)
					wave->type = PT_FLOAT;
				// double
				if (fmt.bitsPerSample == 64)
					wave->type = PT_DOUBLE;
				// not compilant, throw error
				else wave->type = PT_UNSUPPORTED;
				break;
			default:	// some other format, not supported
				wave->type = PT_UNSUPPORTED;
			}
			break;
		case FOURCC('d','a','t','a'):	// copy wave data
#if USE_LINEAR
			if (wave->SamplesPerSec == 37800)
			{
				short *out;
				wave->size = interpolate((short*)&buffer[pos + sizeof(chunk)], &out, chunk.size, wave->NumOfChan, wave->SamplesPerSec);
				wave->data = (unsigned char*)out;
				wave->SamplesPerSec = 44100;
			}
			else
#endif
			{
				wave->data = (unsigned char*)malloc(chunk.size);
				wave->size = chunk.size;
				memcpy(wave->data, &buffer[pos + sizeof(chunk)], chunk.size);
				switch (wave->type)
				{
				case PT_SIGNED16:
					wave->samples = chunk.size / 2;
					break;
				case PT_SIGNED8:
					wave->samples = chunk.size;
					break;
				case PT_FLOAT:
					wave->samples = chunk.size / 4;
					break;
				case PT_DOUBLE:
					wave->samples = chunk.size / 8;
					break;
				}
			}
		default:
			pos += chunk.size + sizeof(chunk);
		}
	}

	free(buffer);
}

short float_to_s16(float sample)
{
	return (short)(sample * 32768.f);
}

short double_to_s16(double sample)
{
	return (short)(sample * 32768.);
}

short u8_to_s16(u8 sample)
{
	return (short)(((s8)sample - 0x7f) << 8);
}

short s8_to_s16(s8 sample)
{
	return (short)(sample << 8);
}

short u16_to_s16(u16 sample)
{
	return (short)((int)sample - 0x7fff);
}

int consume_samples(WAVE *wave, int samples, s16 *dst)
{
	int i = 0;

	switch (wave->type)
	{
		case PT_SIGNED16:
			// simple copy
			for (i = 0; i < samples && wave->consumed < wave->samples; i++)
				dst[i] = ((s16*)wave->data)[wave->consumed++];
			break;
		case PT_SIGNED8:
			// convert
			for (i = 0; i < samples && wave->consumed < wave->samples; i++)
				dst[i] = s8_to_s16((s8)wave->data[wave->consumed++]);
			break;
		case PT_FLOAT:
			// convert
			for (i = 0; i < samples && wave->consumed < wave->samples; i++)
				dst[i] = float_to_s16(((float*)wave->data)[wave->consumed++]);
			break;
		case PT_DOUBLE:
			// convert
			for (i = 0; i < samples && wave->consumed < wave->samples; i++)
				dst[i] = double_to_s16(((double*)wave->data)[wave->consumed++]);
			break;
	}

	// return how many samples are left
	return i;
}

void close_wave(WAVE *wave)
{
	if(wave->data) free(wave->data);
	memset(wave, 0, sizeof(*wave));
}
