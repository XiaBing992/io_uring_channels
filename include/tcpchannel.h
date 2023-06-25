#ifndef TCPCHANNEL_H
#define TCPCHANNEL_H
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <liburing.h>
#include <unordered_map>
#include <vector>
#include "utils.h"
#include "threadpool.h"

class TcpChannel
{
    private:
        std::string m_sIp;
        ConnectionData* m_cSocketData;
        ThreadPool m_cThreadPool; 
        int m_nPort;
        int m_nSocketFd;
        struct io_uring m_cRing;
        std::unordered_map<int, ConnectionData *> m_umConnections;
        void startAccept();
        void startRead(int client_fd);
        void startWrite(int client_fd);
        void setEchoMessage(ConnectionData *data,const std::string str);
        void processEvent(io_uring_cqe *cqe);
        void processAccept(int res);
        void processRead(int client_fd, int res);
        void processWrite(int client_fd, int res);
        void clear(int client_fd);

    public:
        TcpChannel(const std::string ip, const int port, int thread_num);
        void startIouring();
        void run();
        ~TcpChannel();
};

#endif