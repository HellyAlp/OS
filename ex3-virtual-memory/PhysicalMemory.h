#pragma once

#include "MemoryConstants.h"

/*
 * reads an integer from the given physical address and puts it in 'value'
 */
void PMread(uint64_t physicalAddress, word_t* value);

/*
 * writes 'value' to the given physical address
 */
void PMwrite(uint64_t physicalAddress, word_t value);


/*
 * evicts a page from the RAM to the hard drive
 */
void PMevict(uint64_t frameIndex, uint64_t evictedPageIndex);


/*
 * restores a page from the hard drive to the RAM
 */
void PMrestore(uint64_t frameIndex, uint64_t restoredPageIndex);
