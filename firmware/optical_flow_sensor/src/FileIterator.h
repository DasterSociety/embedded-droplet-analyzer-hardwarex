#pragma once

#include <Arduino.h>

// Walks the SD card looking for files named LOG_001.CSV … LOG_999.CSV
// (the format produced by Logger::writeAll). Existing files are processed
// in numerical order; missing slots are skipped.
namespace FileIterator {

void     reset();                                  // restart iteration from slot 1
bool     findFirst(char *filenameOut, size_t len); // first existing slot in 1..999
bool     findNext (char *filenameOut, size_t len); // next existing slot after current
uint16_t currentIndex();                           // last index returned (0 = none yet)

}  // namespace FileIterator
