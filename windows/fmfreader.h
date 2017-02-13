#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cv.h"

class fmfreader {
	#define BITRATEPERPIXEL 5  /* in the compressed output, how many bits per pixel? */
	#define STREAM_GOP_SIZE 100	/* how often to we emit a key frame? */
	#define STREAM_FRAME_RATE 25 	/* frames per second */
	#define STREAM_PIX_FMT PIX_FMT_YUV420P 	/* color encoding for each pixel; most formats seem to like YUV420 */
											/* default pix_fmt */
	
	typedef unsigned int uint32;
	typedef unsigned long long uint64;

public:
	fmfreader();  //default constructor
	~fmfreader();  //deconstructor
	void fmf_open(char* filename);  //sets and opens an fmf movie (must be done first!)
	void fmf_close();  //closes the file pointer to the fmf movie
	uint32 fmf_get_fwidth() { return nc; };  //returns width of frame
	uint32 fmf_get_fheight() { return nr; };  //returns height of frame
	uint64 fmf_get_nframes() { return nframes; };  //returns number of frames in movie
	void fmf_read_header();  //reads in fmf header information, storing in class variables
	void fmf_read_frame(int frame, uint8_t * buf);  //reads in frame number 'frame' into buf
	IplImage* fmf_queryframe();  //functions similarly to cvqueryframe (auto advances 1 frame)
	bool fmf_fp_isopen();	//returns true if fmffp is not NULL

	IplImage* fmf_nextframe();  //read bytes for next frame, picking up immediately where we left off
	void fmf_pointff(); //point to start of first frame (ie. after header)

private:

	FILE * fmffp; 	/* pointer to fmf file stream */
	int fmfnpixels;	/* number of pixels in each frame */
	uint64 headersize;	/* number of bytes in header */
	uint32 version;	/* version of fmf file */
	uint32 nr;	/* number or rows of pixels in each frame */
	uint32 nc;	/* number or columns of pixels in each frame */
	uint64 bytesperchunk;	/* number of bytes to encode each frame */
	uint64 nframes;	/* number of frames */
	uint32 bitsperpixel;	/* number of bits used to encode each pixel */

	int isoddrows;	/* 1 if nr is odd, 0 o.w. This is necessary because the
					video is required to have and even number of rows and
					columns. We pad one column on the right and one column
					on th bottom if necessary. */
	int isoddcols;	/* 1 if nc is odd, 0 o.w. */

	uint8_t * extrabuf;	/* if isoddcols==1, then we will need an extra buffer to read in the unpadded image. */

	//Variables used in buffer conversion to IplImage
	uint64 currframe;
	CvSize size;
	int depth = IPL_DEPTH_8U;
	int channels = 1;
	IplImage* frame;
	uint8_t* imagebuffer;
};
