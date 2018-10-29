#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <vorbis/vorbisenc.h>
#include "dec.h"

#pragma comment(lib, "libogg_static.lib")
#pragma comment(lib, "libvorbis.lib")

#define SPLICE	(16*1024)	// 32 KB

void convert(FILE *in, FILE *out, float quality)
{
	WAVE wav;
	ogg_stream_state os; /* take physical pages, weld into a logical
						 stream of packets */
	ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       op; /* one raw packet of data for decode */

	vorbis_info      vi; /* struct that stores all the static vorbis bitstream
						 settings */
	vorbis_comment   vc; /* struct that stores all the user comments */

	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */

	int ret;

	open_wave(in, &wav);

	vorbis_info_init(&vi);
	ret = vorbis_encode_init_vbr(&vi, wav.NumOfChan, wav.SamplesPerSec, quality);

	/* do not continue if setup failed; this can happen if we ask for a
	mode that libVorbis does not support (eg, too low a bitrate, etc,
	will return 'OV_EIMPL') */

	if (ret)exit(1);

	/* add a comment */
	vorbis_comment_init(&vc);
	//vorbis_comment_add_tag(&vc, "ENCODER", "encoder_example.c");

	/* set up the analysis state and auxiliary encoding storage */
	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	/* set up our packet->stream encoder */
	/* pick a random serial number; that way we can more likely build
	chained streams just by concatenation */
	srand((unsigned int)time(NULL));
	ogg_stream_init(&os, rand());

	/* Vorbis streams begin with three headers; the initial header (with
	most of the codec setup parameters) which is mandated by the Ogg
	bitstream spec.  The second header holds any comment fields.  The
	third header holds the bitstream codebook.  We merely need to
	make the headers, then pass them to libvorbis one at a time;
	libvorbis handles the additional Ogg bitstream constraints */

	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;

		vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
		ogg_stream_packetin(&os, &header); /* automatically placed in its own
										   page */
		ogg_stream_packetin(&os, &header_comm);
		ogg_stream_packetin(&os, &header_code);

		/* This ensures the actual
		* audio data will start on a new page, as per spec
		*/
		while (1)
		{
			int result = ogg_stream_flush(&os, &og);
			if (result == 0)break;
			fwrite(og.header, 1, og.header_len, out);
			fwrite(og.body, 1, og.body_len, out);
		}

	}

	// work with split sizes
	short s16_buf[SPLICE];
	while (1)
	{
		int read = consume_samples(&wav, SPLICE, s16_buf);
		if (read == 0)
		{
			vorbis_analysis_wrote(&vd, 0);
			break;
		}
		long i = 0;
		long bytes = read * 2;

		printf("\r%d/%d                       \r", wav.consumed, wav.samples);

		/* expose the buffer to submit data */
		float **buffer = vorbis_analysis_buffer(&vd, SPLICE * 4);

		/* uninterleave samples */
		s16 *rseek = s16_buf;
		switch (wav.NumOfChan)
		{
		case 1:
			for (i; i < bytes / 2; i++)
				buffer[0][i] = *rseek++ / 32768.f;
			break;
		case 2:
			for (i; i < bytes / 4; i++)
			{
				buffer[0][i] = *rseek++ / 32768.f;
				buffer[1][i] = *rseek++ / 32768.f;
			}
			break;
		}

		/* tell the library how much we actually submitted */
		vorbis_analysis_wrote(&vd, i);

		/* vorbis does some data preanalysis, then divvies up blocks for
		more involved (potentially parallel) processing.  Get a single
		block for encoding now */
		while (vorbis_analysis_blockout(&vd, &vb) == 1)
		{
			/* analysis, assume we want to use bitrate management */
			vorbis_analysis(&vb, NULL);
			vorbis_bitrate_addblock(&vb);

			while (vorbis_bitrate_flushpacket(&vd, &op))
			{

				/* weld the packet into the bitstream */
				ogg_stream_packetin(&os, &op);

				/* write out pages (if any) */
				while (1)
				{
					int result = ogg_stream_pageout(&os, &og);
					if (result == 0) break;
					fwrite(og.header, 1, og.header_len, out);
					fwrite(og.body, 1, og.body_len, out);

					/* this could be set above, but for illustrative purposes, I do
					it here (to show that vorbis does know where the stream ends) */
					if (ogg_page_eos(&og)) break;
				}
			}
		}
	}

	/* clean up and exit.  vorbis_info_clear() must be called last */

	ogg_stream_clear(&os);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);

	close_wave(&wav);
}
