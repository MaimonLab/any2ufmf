#pragma once
#include "cv.h"

double pg_timestamp(IplImage* frame);
uint32_t extract_bits(uint32_t num, uint8_t nbits, uint8_t start);
void printbinary(unsigned n);