//
//  EVLibrary.cpp
//  Scope
//
//  Created by Daniel Vasile on 2019-10-24.
//  Copyright © 2019 EEVengers. All rights reserved.
//

#include "EVLibrary.hpp"

//global variable declerations
volatile bool programClosing;
std::mutex dataProcessingBufferLock;

//global classes defenitions

//---------------EVException---------------
EVException::EVException(int errorCode, const char* subSystem)
    : m_msg(std::string("Exception with error code - ")
            + std::to_string(errorCode)
            + std::string(" - from SubSystem: ")
            + std::string(subSystem)
            )
{}

const char * EVException::what() const throw()
{
    return m_msg.c_str();
}


//---------------EVSharecCache---------------

EVSharedCache::EVSharedCache(unsigned int cacheSize, unsigned int numCaches)
{
    if(cacheSize == 0 || numCaches < 2) throw EVException(EVErrorCodeInvalidValue,"EVSharedCache::Constructor");
    
    this->numCaches = numCaches;
    this->cacheSize = cacheSize;
    caches = (unsigned char**)malloc(sizeof(unsigned char**) * numCaches);
    for(unsigned int i = 0; i < numCaches; i++) {
        caches[i] = (unsigned char*)malloc(sizeof(unsigned char*) * cacheSize);
    }

    this->readCache = 0;
    this->writeCache = 0;
}

void EVSharedCache::SetWriteCache(const unsigned char* buff)
{
    this->lock.lock();

    //copy data
    for(unsigned int i = 0; i < cacheSize; i++)
    {
        caches[writeCache][i] = buff[i];
    }
    //set write cache to next cache
    if(writeCache == numCaches - 1){
        writeCache = 0;
    } else {
        writeCache++;
    }

    this->lock.unlock();
}

void EVSharedCache::PartialSetWriteCache(const unsigned char* buff, unsigned int &idx, unsigned int size) {
    if(idx + size > cacheSize) throw EVException(EVErrorCodeInvalidValue,"EVSharedCache::PartialSetWriteCache()");

    this->lock.lock();

    for(unsigned int i = 0; i < size; i++) {
        caches[writeCache][idx + i] = buff[i];
    }
    idx += size;

    //when this buffer is full
    if(idx == cacheSize) {
        //move onto next buffer
        if(writeCache == numCaches - 1) {
            writeCache = 0;
        } else {
            writeCache++;
        }
        //reset the idx
        idx = 0;
    }
    
    this->lock.unlock();
}

int EVSharedCache::CopyReadCache(unsigned char* buff, unsigned int size)
{
    //if there is no new data to return, exit function
    this->lock.lock();
    
    if(this->writeCache == this->readCache) {
        this->lock.unlock();
        return 1;
    }

    if(this->cacheSize < size) throw EVException(EVErrorCodeInvalidValue,"EVSharedCache::CopyReadCache");

    //copy data
    memcpy(buff,caches[readCache],size);
    //advance to next cache
    if(readCache == numCaches - 1) {
        readCache = 0;
    } else {
        readCache++;
    }

    this->lock.unlock();

    return 0;
}

EVSharedCache::~EVSharedCache()
{
    for(unsigned int i = 0; i < numCaches; i++) {
        free(caches[i]);
    }
    free(caches);
}

//---------------EVCacheCopySignal---------------