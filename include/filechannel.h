#ifndef FILECHANNEL_H
#define FILECHANNEL_H

#include "liburing/io_uring.h"
#include "liburing.h"
#include<iostream>


#define QUEUE_SIZE 1

class FileChannel
{
    private:
    io_uring_params m_sInitParams;
    io_uring m_sIoUring;
    int fd;

    public:
    FileChannel(std::string file_path);
    bool read();
    bool write(const std::string data);
    ~FileChannel(){}

};

#endif


