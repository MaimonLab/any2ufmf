#include "fmfreader.h"

/* fmf_read_header:
Reads the header from stream fmffp and stores information
about the movie in the global variables defined above.
*/
void fmfreader::fmf_read_header() {

	uint32 formatlen;
	int sizeofuint32 = 4;
	int sizeofuint64 = 8;

	/* seek to the start of the movie */
	fseek(fmffp, 0, SEEK_SET);

	/* version number */
	if (fread(&version, sizeofuint32, 1, fmffp) < 1) {
		fprintf(stderr, "Error reading version number of input fmf file.\n");
		exit(1);
	}

	if (version == 1) {
		/* version 1 only formats with MONO8 */
		bitsperpixel = 8;
	}
	else if (version == 3) {
		/* version 3 encodes the length of the format string */
		if (fread(&formatlen, sizeofuint32, 1, fmffp)<1) {
			fprintf(stderr, "Error reading format length of input fmf file.\n");
			exit(1);
		}
		/* followed by the format string */
		fseek(fmffp, formatlen, SEEK_CUR);
		/* followed by the bits per pixel */
		if (fread(&bitsperpixel, sizeofuint32, 1, fmffp)<1) {
			fprintf(stderr, "Error reading bits per pixel of input fmf file.\n");
			exit(1);
		}
	}
	/* all versions then have the height of the frame */
	if (fread(&nr, sizeofuint32, 1, fmffp)<1) {
		fprintf(stderr, "Error reading frame height of input fmf file.\n");
		exit(1);
	}
	/* width of the frame */
	if (fread(&nc, sizeofuint32, 1, fmffp)<1) {
		fprintf(stderr, "Error reading frame width of input fmf file.\n");
		exit(1);
	}
	/* bytes encoding a frame */
	if (fread(&bytesperchunk, sizeofuint64, 1, fmffp)<1) {
		fprintf(stderr, "Error reading bytes per chunk of input fmf file.\n");
		exit(1);
	}
	/* number of frames */
	if (fread(&nframes, sizeofuint64, 1, fmffp)<1) {
		fprintf(stderr, "Error reading number of frames of input fmf file.\n");
		exit(1);
	}
	/* we've now read in the whole header, so use ftell
	to get the header size */
	headersize = ftell(fmffp);
	/* check to see if the number of frames was written */
	if (nframes == 0) {
		/* if not, then seek to the end of the file, and compute number
		of frames based on file size */
		fseek(fmffp, 0, SEEK_END);
		nframes = (ftell(fmffp) - headersize) / bytesperchunk;
	}

	/* compute total number of pixels */
	fmfnpixels = nr*nc;
	/* store whether the number of rows, columns is odd */
	isoddrows = (nr % 2) == 1;
	isoddcols = (nc % 2) == 1;
	/* if number of columns is odd, allocate extrabuf */
	if (isoddcols) {
		/* allocate an extra buffer so that we can pad images */
		extrabuf = (uint8_t*)malloc(sizeof(uint8_t)*fmfnpixels);
	}

	return;

}

/* fmf_read_frame:
Reads frame number "frame" of the stream fmffp into buf.
*/
void fmfreader::fmf_read_frame(int frame, uint8_t * buf) {

	int i;

	/* seek to the start of the frame: size of header + size of
	previous frames + size of timestamp */
	fseek(fmffp, headersize + bytesperchunk*frame + sizeof(double), SEEK_SET);

	/* read in the image */
	if (isoddcols) {
		/* if the number of columns is odd, we are going to pad
		with a column on the right that is 128. first, read
		frame into extra buffer */
		if (fread(extrabuf, sizeof(uint8_t), fmfnpixels, fmffp) < fmfnpixels) {
			exit(1);
		}
		/* then copy over to buf, adding an extra pixel at every row */
		for (i = 0; i < nr; i++) {
			memcpy(buf, extrabuf, nc);
			buf[nc] = 128;
			buf = &buf[nc + 1];
			extrabuf = &extrabuf[nc];
		}
	}
	else {
		/* otherwise, just read in the frame */
		if (fread(buf, sizeof(uint8_t), fmfnpixels, fmffp) < fmfnpixels) {
			exit(1);
		}
	}

	/* if the number of rows is odd, we are going to pad with a row
	on the bottom that is 128 */
	if (isoddrows) {
		for (i = 0; i < nc; i++) {
			buf[fmfnpixels - i - 1] = 128;
		}
	}

	return;

}

