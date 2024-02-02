#ifndef _commons_h_
#define _commons_h_

#include "SimpleTimer.h"

// Get the number of elements in a C-style array 
#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

// streamtitle can max use half of buffer size, because title is doubled for scrolling
#define MAXLEN_SCROLL_STREAMTITLE_BUFFER  128


extern SimpleTimer timer;
extern char scrollingStreamTitleBuffer[];

#endif