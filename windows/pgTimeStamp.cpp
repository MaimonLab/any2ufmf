#include "pgTimeStamp.h"

double pg_timestamp(IplImage* frame)
{
	double ts = 0.0;

	int x = 0;  //column
	int y = 0;  //row
	int cchan = 0;  //color channel of interest

	//Grab each pixel's data
	uint32_t pixel0 = CV_IMAGE_ELEM(frame, unsigned char, y, ((x+0)*3)+cchan);
	uint32_t pixel1 = CV_IMAGE_ELEM(frame, unsigned char, y, ((x+1)*3)+cchan);
	uint32_t pixel2 = CV_IMAGE_ELEM(frame, unsigned char, y, ((x+2)*3)+cchan);
	uint32_t pixel3 = CV_IMAGE_ELEM(frame, unsigned char, y, ((x+3)*3)+cchan);
	
	//printf("pixel values:  %d %d %d %d\n", (int)pixel0, (int)pixel1, (int)pixel2, (int)pixel3);
	//printbinary(pixel0);
	//printf(" | ");
	//printbinary(pixel1);
	//printf(" | ");
	//printbinary(pixel2);
	//printf(" | ");
	//printbinary(pixel3);
	//printf("\n");

	//Shift bits so we can combine
	pixel0 = pixel0 << (32 - 8);
	pixel1 = pixel1 << (32 - 16);
	pixel2 = pixel2 << (32 - 24);

	//printbinary(pixel0);
	//printf(" | ");
	//printbinary(pixel1);
	//printf(" | ");
	//printbinary(pixel2);
	//printf(" | ");
	//printbinary(pixel3);
	//printf("\n");

	//Combine the bits
	uint32_t ts_raw = pixel0 | pixel1 | pixel2 | pixel3;

	//printbinary(ts_raw);
	//printf("\n");

	//Extract bits (0 is right most bit)
	uint32_t second_count = extract_bits(ts_raw, 7, 25);  //was 31
	uint32_t cycle_count = extract_bits(ts_raw, 13, 12);  //was 31-7
	uint32_t cycle_offset = extract_bits(ts_raw, 12, 0);
	
	//printbinary(second_count);
	//printf(" | ");
	//printbinary(cycle_count);
	//printf(" | ");
	//printbinary(cycle_offset);
	//printf("\n");

	//Convert to single ts
	ts = second_count + (cycle_count + cycle_offset / 3072.0) / 8000.0;

	//printf("%d, %d, %d\n", (int)second_count, (int)cycle_count, (int)cycle_offset);
	//printf("%f\n", ts);

	return ts;
}

uint32_t extract_bits(uint32_t num, uint8_t nbits, uint8_t start)
{
	uint32_t mask = ((1 << nbits) - 1) << start;  // create mask
	return (num & mask) >> start;
}

void printbinary(unsigned n)
{
	for (int i = 31; i >= 0; i--)
	{
		std::cout << ((n >> i) & 1);
	}
}