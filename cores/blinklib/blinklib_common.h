#ifndef BLINKLIB_COMMON_H_
#define BLINKLIB_COMMON_H_

// Number of faces on a blink. Looks nicer than hardcoding '6' everywhere.
#define FACE_COUNT 6

// 'Cause C ain't got no iterators and all those FOR loops are too ugly.
// TODO: Yuck, gcc expands this loop index to a word, which costs a load, a
// compare, and a multiply. :/
#define FOREACH_FACE(x)            \
  for (byte x = 0; x < FACE_COUNT; \
       ++x)  // Pretend this is a real language with iterators

// Get the number of elements in an array.
#define COUNT_OF(x) ((sizeof(x) / sizeof(x[0])))

#endif