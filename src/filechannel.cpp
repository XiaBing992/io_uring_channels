#include<iostream>
#include<sys/stat.h>
#include<stdlib.h>
#include<unistd.h>
#include "filechannel.h"

FileChannel::FileChannel(std::string file_path)
{
    int ret=io_uring_queue_init(QUEUE_SIZE,&m_sIoUring,0);

    if(ret<0)
    {
        std::cerr<<"io_uring_queue_init error."<<std::endl;
    }
    //获取文件描述符
    fd=open(file_path.c_str(),O_RDWR);

    if(fd<0)
    {
        std::cerr<<"open error."<<std::endl;
    }
}
bool FileChannel::read()
{
    struct stat fstat_param;
    if(fstat(fd,&fstat_param)<0)
    {
        std::cerr<<"fstat error."<<std::endl;
    }
    struct iovec *buffers=(struct iovec*)calloc(QUEUE_SIZE,sizeof(struct iovec));
    
    //给队列缓冲区分配内存
    int fsize=0;
    for(int i=0;i<QUEUE_SIZE;i++)
    {
        void *buf;
        if(posix_memalign(&buf,4096,4096)!=0)
        {
            std::cerr<<"poisx_menalign error."<<std::endl;
            exit(1);
        }
        buffers[i].iov_base=buf;
        buffers[i].iov_len=4096;
        fsize+=4096;
    }

    //初始化sqe信息
    struct io_uring_sqe *sqe;
    int offset=0;
    for(int i=0;;i++)
    {
        //取sq队列头任务
        sqe=io_uring_get_sqe(&m_sIoUring);
        if(!sqe)
        {
            break;
        }
        //std::cout<<i<<std::endl;
        io_uring_prep_readv(sqe,fd,&buffers[i],1,offset);
        //std::cout<<(int)sqe->opcode<<std::endl;
        offset+=buffers[i].iov_len;
        //std::cout<<buffers[i].iov_len<<std::endl;
        //std::cout<<fstat_param.st_size<<std::endl;
        if(offset>fstat_param.st_size)
        {
            break;
        }
    }

    //提交任务
    int ret=io_uring_submit(&m_sIoUring);

    if(ret<0)
    {
        std::cerr<<"io_uring_submit error."<<std::endl;
    }
    int done=0;
    int pedding=ret;
    fsize=0;
    struct io_uring_cqe *cqe;
    for(int i=0;i<pedding;i++)
    {
        //等待任务执行
        ret=io_uring_wait_cqe(&m_sIoUring,&cqe);

        if(ret<0)
        {
            std::cerr<<"io_uring_wait_cqe error."<<std::endl;
        }
        done++;
        ret=0;
        if(cqe->res!=4096&&cqe->res + fsize != fstat_param.st_size)
        {
            std::cerr<<"error.."<<cqe->res<<std::endl;
            ret=1;
        }
        fsize+=cqe->res;
        //更新cq队列
        io_uring_cqe_seen(&m_sIoUring,cqe);

        if(ret)
        {
            break;
        }
    }

    //std::cout<<done<<" "<<pedding<<" "<<fsize<<std::endl;

    return true;
}

bool FileChannel::write(const std::string data)
{
    //获取文件描述符
    struct stat fstat_param;
    if(fstat(fd,&fstat_param)<0)
    {
        std::cerr<<"fstat error."<<std::endl;
    }
    struct iovec *buffers=(struct iovec*)calloc(QUEUE_SIZE,sizeof(struct iovec));

    //给队列缓冲区分配内存
    int fsize=0;
    const int data_size=data.size()+1;
    for(int i=0;i<QUEUE_SIZE;i++)
    {
        void *buf;
        if(posix_memalign(&buf,128,data_size)!=0)
        {
            std::cerr<<"poisx_menalign error."<<std::endl;
            exit(1);
        }
        char *temp=(char*)buf;
        for(int k=0;k<data_size;k++)
        {
            if(k==data_size-1)
            {
                temp[k]='\n';
            }
            else
            {
                temp[k]=data[k];
            }
        }
        buffers[i].iov_base=buf;
        buffers[i].iov_len=data_size;
        fsize+=data_size;
    }

    //初始化sqe信息
    struct io_uring_sqe *sqe;
    int offset=0;
    for(int i=0;;i++)
    {
        //取sq队列头任务
        sqe=io_uring_get_sqe(&m_sIoUring);
        if(!sqe)
        {
            break;
        }
        io_uring_prep_writev(sqe,fd,&buffers[i],1,offset);
        offset+=buffers[i].iov_len;
    }


    //提交任务
    int ret=io_uring_submit(&m_sIoUring);
    //std::cout<<ret<<std::endl;

    if(ret<0)
    {
        std::cerr<<"io_uring_submit error."<<std::endl;
    }
    int done=0;
    int pedding=ret;
    fsize=0;
    struct io_uring_cqe *cqe;
    for(int i=0;i<pedding;i++)
    {
        //等待任务执行
        ret=io_uring_wait_cqe(&m_sIoUring,&cqe);

        if(ret<0)
        {
            std::cerr<<"io_uring_wait_cqe error."<<std::endl;
        }
        done++;
        ret=0;
        if(cqe->res!=data_size&&cqe->res + fsize != fstat_param.st_size)
        {
            std::cerr<<"error.."<<cqe->res<<std::endl;
            ret=1;
        }
        fsize+=cqe->res;
        //更新cq队列
        io_uring_cqe_seen(&m_sIoUring,cqe);

        if(ret)
        {
            break;
        }
    }
    return true;
}