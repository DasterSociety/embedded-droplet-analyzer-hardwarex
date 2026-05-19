#include <SD.h>

#include "FileIterator.h"

namespace {

uint16_t g_idx = 0;  // last index returned (0 = none yet)

bool slotExists(uint16_t i, char *buf, size_t len) {
    snprintf(buf, len, "LOG_%03u.CSV", i);
    return SD.exists(buf);
}

}  // namespace

namespace FileIterator {

void reset() { g_idx = 0; }

bool findFirst(char *filenameOut, size_t len) {
    for (uint16_t i = 1; i <= 999; i++) {
        if (slotExists(i, filenameOut, len)) {
            g_idx = i;
            return true;
        }
    }
    return false;
}

bool findNext(char *filenameOut, size_t len) {
    for (uint16_t i = (uint16_t)(g_idx + 1u); i <= 999; i++) {
        if (slotExists(i, filenameOut, len)) {
            g_idx = i;
            return true;
        }
    }
    return false;
}

uint16_t currentIndex() { return g_idx; }

}  // namespace FileIterator
