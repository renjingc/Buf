/**
 * @author your name and id
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"

namespace badgerdb {

BufMgr::BufMgr(std::uint32_t bufs)
    : numBufs(bufs) {
     bufDescTable = new BufDesc[bufs];

    for (FrameId i = 0; i < bufs; i++)
    {
        bufDescTable[i].frameNo = i;
        bufDescTable[i].valid = false;
    }

    bufPool = new Page[bufs];

    int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
    hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

    clockHand = bufs - 1;
}

/**
destructor
*/
BufMgr::~BufMgr() {
    delete [] bufDescTable;
    delete [] bufPool;
}


/**
* @brief advanceClock
*       Iterate the clockHand with wraparound
*/
void BufMgr::advanceClock()
{
    //the clockhand starts at the last
    //position, so first iteration
    //will advance to position 0
    clockHand = (clockHand+1) % numBufs;
}

/**
* @brief allocBuf
*       Allocate a frame using the clock algorithm
*       Can write a page back to disk
*       If all frames pinned, throws BufferExceeded
*       Exception
*       This is a private method
* @param [in] frame     frame num
*/
void BufMgr::allocBuf(FrameId & frame)
{
    //If all frames pinned, can't proceed.
    int numPinned=0;
    int bufs = numBufs;
    for (int i=0; i < bufs; i++) {
        if (bufDescTable[i].pinCnt > 0)
            numPinned++;
    }
    if (numPinned == bufs) {
        throw BufferExceededException();
    }

    //find a free frame
    BufDesc* tmpbuf;
    int numScanned = 0;
    bool found = 0;
    while (numScanned < 2*bufs)
    {
        advanceClock();
        tmpbuf = &bufDescTable[clockHand];
        // if invalid, use frame
        if (! tmpbuf->valid )
        {
            break;
        }

        // if invalid, check referenced bit
        if (! tmpbuf->refbit)
        {
            // check to see if someone has it pinned
            if (tmpbuf->pinCnt == 0)
            {
                // no one has it pinned, so use it.

                hashTable->remove(tmpbuf->file,tmpbuf->pageNo);
                // increment pin and set referenced
                found = true;
                break;
            }
        }
        else
        {
            // has been referenced, clear bit and continue
            tmpbuf->refbit = false;
        }

    }

    // throw if buffer has no available slots
    if (!found && numScanned >= 2*bufs)
    {
        throw BufferExceededException();
    }

    // flush page changes to disk
    if (tmpbuf->dirty)
    {
        tmpbuf->file->writePage(bufPool[clockHand]);
    }

    // return frame number
    frame = clockHand;
}

/**
* @brief readPage
*       Given a file object and page number, will return addr of page in buf pool
*       Caller won't know if it had to be io'd
*  @param [in] file     point to file
*  @param [in] PageNo   pageId
*  @param [in] page     point to page
*/
void BufMgr::readPage(File* file, const PageId PageNo, Page*& page)
{
    FrameId frameNo=0;

    try
    {
        //find this page in the existing buffer pool

        hashTable->lookup(file, PageNo, frameNo);

        bufDescTable[frameNo].refbit = true;
        bufDescTable[frameNo].pinCnt++;
        page=&bufPool[frameNo];//ret val

    }
    catch (HashNotFoundException)
    {
        //allocate new space for this file and page.

        //io pg first which tests file and pg for validity
        //we'll let the InvalidPageException to percolate up.
        Page p;
        //io pg
        p = file->readPage(PageNo);

        //Valid page:
        //get frameno
        BufMgr::allocBuf(frameNo);
        //store page
        bufPool[frameNo] = p;
        //know where it is
        hashTable->insert(file, PageNo, frameNo);

        //set new frame up
        bufDescTable[frameNo].Set(file, PageNo);
        //ret val
        page = &bufPool[frameNo];
    }
}

/**
* @brief unPinPage
*       Given the file and pageNo, will unpin the page and set dirty
*       If the file and page isn't in the buffer pool, throw HashNotFoundException
*  @param [in] file     point to file
*  @param [in] PageNo   pageId
*  @param [in] dirty    is or not dirty
*/
void BufMgr::unPinPage(File* file, const PageId PageNo, const bool dirty)
{
    // lookup in hashtable
    FrameId frameNo = 0;
    hashTable->lookup(file, PageNo, frameNo);
    if (dirty == true)
        bufDescTable[frameNo].dirty = dirty;
    // make sure the page is actually pinned
    if (bufDescTable[frameNo].pinCnt == 0)
    {
        throw PageNotPinnedException(file->filename(), PageNo, frameNo);
    }
    else
        bufDescTable[frameNo].pinCnt--;
}

/**
*  @brief flushFile
*       Writes the file to the disk.
*       Iterates over all page frames, flushing the (dirty)
*       ones that belong to the file to disk.
*  @param [in] file     point to file
*/
void BufMgr::flushFile(const File* file)
{
    BufDesc* tmpbuf;
    for (std::uint32_t i=0; i < numBufs; i++)
    {
        tmpbuf = &(bufDescTable[i]);

        // make sure we have a page in correct file
        bool invalid = !tmpbuf->valid;
        bool correctFile = (tmpbuf->file == file);
        if (!correctFile) {
            continue;
        }


        // Correct file, proceed
        // buffer can't be invalid and have a file
        if (invalid) {
            throw BadBufferException(tmpbuf->frameNo,
                        tmpbuf->dirty,
                        tmpbuf->valid,
                        tmpbuf->refbit);
        }


        //valid and correct
        // Good to go write this page.

        //Can't write a pinned page!
        if (tmpbuf->pinCnt > 0)
        {
            throw PagePinnedException(
                file->filename(),
                tmpbuf->pageNo,
                tmpbuf->frameNo);
        }

        if (tmpbuf->dirty)
        {
            FrameId frameNo = tmpbuf->frameNo;
            File* f = bufDescTable[frameNo].file;
            f->writePage(bufPool[frameNo]);
            tmpbuf->dirty = false;
        }
        //no longer keep page
        hashTable->remove(file, tmpbuf->pageNo);
        //clear buffer frame
        bufDescTable[i].Clear();
    }
}
/**
*  @brief allocPage
*  @param [in] file     point to file
*  @param [in] PageNo   pageId
*  @param [in] page     point to page
*/
void BufMgr::allocPage(File* file, PageId& PageNo, Page*& page)
{
    Page oPg = file->allocatePage();
    FrameId frameNo = 0;
    allocBuf(frameNo);
    PageNo = oPg.page_number();
    hashTable->insert(file, PageNo, frameNo); //record that we have this
    bufDescTable[frameNo].Set(file, PageNo); //record this usage in our buffer pool.

    //	memcpy((void*) &bufPool[frameNo], (void*) &oPg, sizeof(Page));
    bufPool[frameNo] = oPg;
    page = &bufPool[frameNo];
}

/**
*  @brief disposePage
*       Deletes a particular page from
*       a file. If there was a frame,
*       clears the frame and hashtable first
*  @param [in] file     point to file
*  @param [in] PageNo   pageId
*/
void BufMgr::disposePage(File* file, const PageId PageNo)
{
    //Deallocate from file altogether
    //See if it is in the buffer pool
    FrameId frameNo = 0;
    hashTable->lookup(file, PageNo, frameNo);
    // clear the page
    bufDescTable[frameNo].Clear();
    hashTable->remove(file, PageNo);
    // deallocate it in the file
    file->deletePage(PageNo);
}

void BufMgr::printSelf(void)
{
    BufDesc* tmpbuf;
    int validFrames = 0;

    for (std::uint32_t i = 0; i < numBufs; i++)
    {
        tmpbuf = &(bufDescTable[i]);
        std::cout << "FrameNo:" << i << " ";
        tmpbuf->Print();

        if (tmpbuf->valid == true)
            validFrames++;
    }

    std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
}

}
