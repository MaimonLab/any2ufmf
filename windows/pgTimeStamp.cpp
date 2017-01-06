#include "pgTimeStamp.h"

//#include "opencv_headers\opencv\opencv2\highgui\highgui.hpp";
//#include "opencv_headers\opencv\opencv2\core\core.hpp";

double pg_timestamp(IplImage* frame)
{
	double ts = 0.0;

	IplImage* ts_pixels;
	//cv::Rect roi_rect(0, 0, 4, 1);

	int row = 0;
	int col = 0;

	//Grab each pixel's data
	uint32_t pixel0 = CV_IMAGE_ELEM(frame, unsigned char, row, col+0);
	uint32_t pixel1 = CV_IMAGE_ELEM(frame, unsigned char, row, col+1);
	uint32_t pixel2 = CV_IMAGE_ELEM(frame, unsigned char, row, col+2);
	uint32_t pixel3 = CV_IMAGE_ELEM(frame, unsigned char, row, col+3);

	printf("pixel values:  %d %d %d %d\n", pixel0, pixel1, pixel2, pixel3);

	//Combine the bits
	pixel0 = pixel0 << (32 - 8);
	pixel1 = pixel1 << (32 - 16);
	pixel2 = pixel2 << (32 - 24);

	uint32_t ts_raw = pixel0 | pixel1 | pixel2 | pixel3;

	//Extract bits
	uint32_t second_count = extract_bits(ts_raw, 7, 32);
	uint32_t cycle_count = extract_bits(ts_raw, 13, 32 - 7);
	uint32_t cycle_offset = extract_bits(ts_raw, 12, 32 - 7 - 13);

	//Shift bits to get actual decimals
	//second_count = second_count >>

	//cv::Mat roi = frame(roi_rect);

	//cv::imshow("Image", roi);
	//cv::waitKey();

	return ts;
}

uint32_t extract_bits(uint32_t num, uint8_t nbits, uint8_t start)
{
	uint32_t mask = ((1 << nbits) - 1) << start;  // create mask
	return (num & mask) >> start;
}