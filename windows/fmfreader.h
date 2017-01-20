#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

class fmfreader {
	/* in the compressed output, how many bits per pixel? */
	#define BITRATEPERPIXEL 5
	/* how often to we emit a key frame? */
	#define STREAM_GOP_SIZE 100
	/* frames per second */
	#define STREAM_FRAME_RATE 25 
	/* color encoding for each pixel; most formats seem to like
	YUV420 */
	#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */
	/* fmf input */
	typedef unsigned int uint32;
	typedef unsigned long long uint64;

public:
	void fmf_read_header();  //reads in fmf header information, storing in class variables
	void fmf_read_frame(int frame, uint8_t * buf);  //reads in frame number 'frame' into buf

private:
	/* pointer to fmf file stream */
	FILE * fmffp;
	/* number of pixels in each frame */
	int fmfnpixels;
	/* number of bytes in header */
	uint64 headersize;
	/* version of fmf file */
	uint32 version;
	/* number or rows of pixels in each frame */
	uint32 nr;
	/* number or columns of pixels in each frame */
	uint32 nc;
	/* number of bytes to encode each frame */
	uint64 bytesperchunk;
	/* number of frames */
	uint64 nframes;
	/* number of bits used to encode each pixel */
	uint32 bitsperpixel;
	/* 1 if nr is odd, 0 o.w. This is necessary because the
	video is required to have and even number of rows and
	columns. We pad one column on the right and one column
	on th bottom if necessary. */
	int isoddrows;
	/* 1 if nc is odd, 0 o.w. */
	int isoddcols;
	/* if isoddcols==1, then we will need an extra buffer to
	read in the unpadded image. */
	uint8_t * extrabuf;
};
