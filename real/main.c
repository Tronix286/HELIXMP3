/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/**************************************************************************************
 * Fixed-point MP3 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * June 2003
 *
 * main.c - command-line test app that uses C interface to MP3 decoder
 **************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "mp3dec.h"

#define READBUF_SIZE		(1024L*16)	/* feel free to change this, but keep big enough for >= one frame at high bitrates */
//#define MAX_ARM_FRAMES		100
//#define ARMULATE_MUL_FACT	1

const char * const DecoderErrorStatusString[] = {
	"ERR_MP3_NONE",
	"ERR_MP3_INDATA_UNDERFLOW",
	"ERR_MP3_MAINDATA_UNDERFLOW",
	"ERR_MP3_FREE_BITRATE_SYNC",
	"ERR_MP3_OUT_OF_MEMORY",
	"ERR_MP3_NULL_POINTER",
	"ERR_MP3_INVALID_FRAMEHEADER",
	"ERR_MP3_INVALID_SIDEINFO",
	"ERR_MP3_INVALID_SCALEFACT",
	"ERR_MP3_INVALID_HUFFCODES",
	"ERR_MP3_INVALID_DEQUANTIZE",
	"ERR_MP3_INVALID_IMDCT",
	"ERR_MP3_INVALID_SUBBAND"
};

void format_elapsed_time(char *time_str, double elapsed) {
    int32_t h, m, s, ms;

    h = m = s = ms = 0;
    ms = elapsed * 1000; // promote the fractional part to milliseconds
    h = ms / 3600000;
    ms -= (h * 3600000);
    m = ms / 60000;
    ms -= (m * 60000);
    s = ms / 1000;
    ms -= (s * 1000);
    sprintf(time_str, "%02li:%02li:%02li.%03li", h, m, s, ms);
}

static int write_little_endian_uint16(FILE *f, uint16_t x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF
	;
}


static int write_little_endian_uint32(FILE *f, uint32_t x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x >> 16, f) != EOF &&
		fputc(x >> 24, f) != EOF
	;
}

static int FillReadBuffer(unsigned char *readBuf, unsigned char *readPtr, int32_t bufSize, int32_t bytesLeft, FILE *infile)
{
	int32_t nRead;

	/* move last, small chunk from end of buffer to start, then fill with new data */
	memmove(readBuf, readPtr, bytesLeft);				
	nRead = fread(readBuf + bytesLeft, 1, bufSize - bytesLeft, infile);
	/* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
	if (nRead < bufSize - bytesLeft)
		memset(readBuf + bytesLeft + nRead, 0, bufSize - bytesLeft - nRead);	

	return nRead;
}

static void WriteWaveHeader(uint32_t total_bytes, uint32_t nChans, uint32_t samprate, uint32_t bitsPerSample, FILE*f)
{
	uint32_t total_size;
	total_size = total_bytes * nChans * samprate * (bitsPerSample / 8);
	if(
		fwrite("RIFF", 1, 4, f) < 4 ||
		!write_little_endian_uint32(f, total_size) ||
		fwrite("WAVEfmt ", 1, 8, f) < 8 ||
		!write_little_endian_uint32(f, 16) ||
		!write_little_endian_uint16(f, 1) ||
		!write_little_endian_uint16(f, (uint16_t)nChans) ||
		!write_little_endian_uint32(f, samprate) ||
		!write_little_endian_uint32(f, samprate * nChans * (bitsPerSample / 8)) ||
		!write_little_endian_uint16(f, (uint16_t)(nChans * (bitsPerSample / 8))) || /* block align */
		!write_little_endian_uint16(f, (uint16_t)bitsPerSample) ||
		fwrite("data", 1, 4, f) < 4 ||
		!write_little_endian_uint32(f, total_size-36)
	) {
		printf("ERROR: write error\n");
	}
}

int main(int argc, char **argv)
{
	int32_t bytesLeft, nRead, err, offset, outOfData, eofReached;
	unsigned char readBuf[READBUF_SIZE], *readPtr;
	short outBuf[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
	FILE *infile, *outfile;
	MP3FrameInfo mp3FrameInfo;
	HMP3Decoder hMP3Decoder;
	int32_t nFrames;
	time_t start_time,end_time;
	char tmp[80];

	printf("MP3 decoder 16bit v1.01     (c) Tronix 2022\n");
	printf("Helix MP3 decoder Copyright (c) 1995-2004 RealNetworks\n\n");

	if (argc != 3) {
		printf("usage: mp3dec infile.mp3 outfile.pcm\n");
		return -1;
	}
	infile = fopen(argv[1], "rb");
	if (!infile) {
		printf("file open error\n");
		return -1;
	}
		
	if (strcmp(argv[2], "nul")) {
		outfile = fopen(argv[2], "wb");
		if (!outfile) {
			printf("file open error\n");
			return -1;
		}
	} else {
		outfile = 0;	/* nul output */
	}


	if ( (hMP3Decoder = MP3InitDecoder()) == 0 ) {
		printf("no free memory\n");
		return -2;
	}

	bytesLeft = 0;
	outOfData = 0;
	eofReached = 0;
	readPtr = readBuf;
	nRead = 0;
	nFrames = 0;
	start_time = time(NULL);

	do {
		/* somewhat arbitrary trigger to refill buffer - should always be enough for a full frame */
		if (bytesLeft < 2*MAINBUF_SIZE && !eofReached) {
			nRead = FillReadBuffer(readBuf, readPtr, READBUF_SIZE, bytesLeft, infile);
			bytesLeft += nRead;
			readPtr = readBuf;
			if (nRead == 0)
				eofReached = 1;
		}

		/* find start of next MP3 frame - assume EOF if no sync found */
		offset = MP3FindSyncWord(readPtr, bytesLeft);
		if (offset < 0) {
			outOfData = 1;
			break;
		}
		readPtr += offset;
		bytesLeft -= offset;


		/* decode one MP3 frame - if offset < 0 then bytesLeft was less than a full frame */
		//startTime = ReadTimer();
 		err = MP3Decode(hMP3Decoder, &readPtr, &bytesLeft, outBuf, 0);
 		
 		//endTime = ReadTimer();
 		//diffTime = CalcTimeDifference(startTime, endTime);
		//totalDecTime += diffTime;

		//printf("frame %5d  \r", 
		//	nFrames);
		fflush(stdout);

		if (err) {
			printf("err occur: %s\n",DecoderErrorStatusString[abs(err)]);
			/* error occurred */
			switch (err) {
			case ERR_MP3_INDATA_UNDERFLOW:
				outOfData = 1;
				break;
			case ERR_MP3_MAINDATA_UNDERFLOW:
				/* do nothing - next call to decode will provide more mainData */
				break;
			case ERR_MP3_FREE_BITRATE_SYNC:
			default:
				outOfData = 1;
				break;
			}
		} else {
 			nFrames++;
			/* no error */
			MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);

			/* write WAVE header before we write the first frame */
			if(nFrames == 1) {
				WriteWaveHeader(0, mp3FrameInfo.nChans, mp3FrameInfo.samprate, mp3FrameInfo.bitsPerSample, outfile);
			}

			if (outfile)
				fwrite(outBuf, mp3FrameInfo.bitsPerSample / 8, mp3FrameInfo.outputSamps, outfile);
			printf("nFrames = %ld, output samps = %ld, sampRate = %ld, nChans = %ld\r", nFrames, mp3FrameInfo.outputSamps, mp3FrameInfo.samprate, mp3FrameInfo.nChans);
		}

	} while ((!outOfData) && (!kbhit()));


	MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
	printf("\ntotal size = %lu\n",(nFrames * mp3FrameInfo.outputSamps * (mp3FrameInfo.bitsPerSample / 8))-44);

	fseek(outfile, 0L, 0);         /*-- pos to header --*/
	
	WriteWaveHeader((nFrames * mp3FrameInfo.outputSamps * (mp3FrameInfo.bitsPerSample / 8))-44, mp3FrameInfo.nChans, mp3FrameInfo.samprate, mp3FrameInfo.bitsPerSample, outfile);

	MP3FreeDecoder(hMP3Decoder);

	fclose(infile);
	if (outfile)
		fclose(outfile);

	end_time=time(NULL);
	format_elapsed_time(tmp,difftime(end_time,start_time));
	fprintf(stderr,"Elapsed time: %s\n",tmp);

	return 0;
}
