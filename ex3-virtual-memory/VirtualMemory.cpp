#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <algorithm>

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

/**
 * Initialize the virtual memory
 */
void VMinitialize() {
    clearTable(0);
}


///////////////////// added functions ////////////////////////

/**
 * Function to extract i bits from j position
 * @return the extracted value as int
 */
int bits(int num, int i, int j)
{
    int result =  (((1 << i) - 1) & (num >> (j - 1)));
    return result;
}


/**
 * A method that checks if the frame is empty (all the rows are 0)
 * @return true if empty, false otherwise
 */
bool isFrameEmpty(uint64_t frame)
{
    int data;
    for(int i=0; i < PAGE_SIZE; i++)
    {
        PMread(frame*PAGE_SIZE+i,&data);
        if (data != 0)
        {
            return false;
        }
    }
    return true;
}

/**
 * update max frame number
 */
void maxUpdate(uint64_t &maxFrame, uint64_t frame)
{
    if (frame > maxFrame)
    {
        maxFrame = frame;
    }
}

/**
 * find a father of a given frame and its index.
 */
void findFather(uint64_t &index, uint64_t value, uint64_t frame, uint64_t currentFather,uint64_t currentIndex,
                uint64_t &father, int layer)
{
    if (value == frame)
    {
        index = currentIndex;
        father = currentFather;
        return;
    }
    if (layer == TABLES_DEPTH )
    {
        return;
    }
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        word_t son;
        PMread(frame * PAGE_SIZE + i, &son);
        if (son != 0)
        {
            findFather(index,value, son, frame,i, father, layer + 1);
        }
    }
}


/**
 * dfs function that traversal through the frames tree and updates the frame according to the priorities
 */
void dfsSearch(uint64_t frame,uint64_t &emptyFrame, uint64_t &maxFrame, uint64_t &maxWeightFrame, uint64_t &maxWeight,
               uint64_t &pageToReturn,uint64_t page, uint64_t frameWeight,uint64_t dontchoose,int layer)
{
    if (layer == TABLES_DEPTH)
    {
        uint64_t pageWeight = page % 2 == 0 ? WEIGHT_EVEN : WEIGHT_ODD;
        frameWeight += pageWeight;
        if(frameWeight > maxWeight && dontchoose != frame)
        {
            maxWeight = frameWeight;
            maxWeightFrame = frame;
            pageToReturn = page;
        }
        maxUpdate(maxFrame,frame);
        return;
    }
    bool isfather = true;
    for (int i = 0; i < PAGE_SIZE; i++) {
        word_t son;
        PMread(frame*PAGE_SIZE+i,&son);
        uint64_t weightToAdd = frame % 2 == 0 ? WEIGHT_EVEN  : WEIGHT_ODD ;
        if (son != 0)
        {
            isfather = false;
            frameWeight = frameWeight + weightToAdd;
            dfsSearch(son, emptyFrame,maxFrame,maxWeightFrame,maxWeight,pageToReturn,(page<<OFFSET_WIDTH)+i,frameWeight,
                      dontchoose,layer+1);
        }
    }
    if(isfather)
    {
        if(frameWeight > maxWeight && dontchoose != frame)
        {
            maxWeight = frameWeight;
            maxWeightFrame = frame;
            pageToReturn = page;
            emptyFrame = maxWeightFrame;
            return;
        }
    }
    if(emptyFrame==0 && isFrameEmpty(frame)  && frame!= dontchoose)
    {
        emptyFrame = frame;
    }
    maxUpdate(maxFrame,frame);
}


/**
 * finds a frame
 * @param dontChoose frame we do not want to evict
 * @return frame
 */
uint64_t find(uint64_t dontChoose)
{
    uint64_t rootFrame = 0;
    uint64_t emptyFrame = 0;
    uint64_t maxFrame = 0;
    uint64_t maxWeightFrame = 0;
    uint64_t maxWeight = 0;
    uint64_t page = 0;
    uint64_t pageToReturn = 0;
    int layer =0;

    dfsSearch(rootFrame,emptyFrame,maxFrame,maxWeightFrame,maxWeight,pageToReturn,page,0,dontChoose,layer);
    if (emptyFrame!=0)
    {
        uint64_t f = 0;
        uint64_t index  = 0 ;
        findFather(index,emptyFrame,0,0,0,f,0);
        PMwrite(f * PAGE_SIZE + index, 0);
        return emptyFrame;
    }
    else if(maxFrame + 1< NUM_FRAMES)
    {
        return maxFrame+1;
    }
    else
    {
        uint64_t f = 0;
        uint64_t index  = 0 ;
        findFather(index,maxWeightFrame,0,0,0,f,0);
        PMwrite(f * PAGE_SIZE + index, 0);
        PMevict(maxWeightFrame,(pageToReturn));
        return  maxWeightFrame;
    }
}


/**
 * A method that gets the virtual address and returns the physical address that suits it.
 * @param virt virtual address
 * @return physical address
 */
uint64_t address(uint64_t virt)
{
    word_t currentTable = 0;
    word_t addr1, f1;
    f1 = 0 ;
    uint64_t indexes[TABLES_DEPTH + 1];
    int j = 1;
    for (int i = TABLES_DEPTH; i >= 0; i--)
    {
        int offset = bits(virt, OFFSET_WIDTH, j);
        j += OFFSET_WIDTH;
        indexes[i] = offset;
    }
    for(int i =0; i < TABLES_DEPTH; i++)
    {
        PMread(currentTable * PAGE_SIZE + indexes[i], &addr1);
        if(addr1 == 0)
        {
            f1 = find(currentTable);
            if(i+1==TABLES_DEPTH)
            {
                PMrestore(f1,(virt>>OFFSET_WIDTH));
            }
            else
            {
                clearTable(f1);
            }
            PMwrite(currentTable*PAGE_SIZE + indexes[i], f1);
            currentTable = f1;
        }
        else
        {
            currentTable = addr1;
        }
    }
    return (currentTable*PAGE_SIZE + indexes[TABLES_DEPTH]);
}




//////////////////// functions to implement /////////////////////////////
/**
 * reads a word from the given virtual address
 * and puts its content in *value.
 *
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMread(uint64_t virtualAddress, word_t* value) {
    if(virtualAddress < VIRTUAL_MEMORY_SIZE)
    {
        PMread(address(virtualAddress),value);
        return 1;
    }
    return 0;
}

/**
 * writes a word to the given virtual address
 * returns 1 on success.
 * returns 0 on failure (if the address cannot be mapped to a physical
 * address for any reason)
 */
int VMwrite(uint64_t virtualAddress, word_t value) {
    if(virtualAddress < VIRTUAL_MEMORY_SIZE)
    {
        PMwrite(address(virtualAddress),value);
        return 1;
    }
    return 0;
}
