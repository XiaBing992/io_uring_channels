#include "tcpchannel.h"

void TcpChannel::startAccept()
{
    //初始化sq队列
    struct io_uring_sqe *sqe = io_uring_get_sqe(&m_cRing);
    io_uring_prep_accept(sqe, m_nSocketFd, (sockaddr *)&(m_cSocketData->addr.ipv4_addr), &(m_cSocketData->addr.lens), 0);
    io_uring_sqe_set_data(sqe, m_cSocketData);
    io_uring_submit(&m_cRing);
}

void TcpChannel::run()
{
    //建立accept连接
    startAccept();

    while (true)
    {
        io_uring_cqe *cqe;
        //等待接受数据
        int ret = io_uring_wait_cqe(&m_cRing, &cqe);
        if (ret < 0)
        {
            break;
        }
        processEvent(cqe);
        //清楚cq
        io_uring_cqe_seen(&m_cRing, cqe);
        //处理多个请求
        while (io_uring_peek_cqe(&m_cRing, &cqe) > 0)
        {
            processEvent(cqe);
            io_uring_cqe_seen(&m_cRing, cqe);
        }
    }
}

//处理请求
void TcpChannel::processAccept(int clientfd)
{
    if (clientfd <= 0)
    {
        return;
    }

    ConnectionData *data = new ConnectionData;
    m_umConnections.insert({clientfd, data});
    startRead(clientfd);
    startAccept();
}
// void TcpChannel::process_read(int client_fd, int res)
// {
//     if (res <= 0)
//     {
//         cleanup(client_fd);
//         std::cout << "delete" << client_fd << std::endl;
//         return;
//     }
//     start_write(client_fd, res);
// }
// void TcpChannel::process_write(int client_fd, int res)
// {
//     //    ConnectionData* data = connections_[client_fd];
//     //     data->isRead = true;
//     //     data->bytesProcessed = 0;
//     //     start_read(client_fd);
//     cleanup(client_fd);
// }
void TcpChannel::startRead(int client_fd)
{
    ConnectionData *data = m_umConnections[client_fd];
    data->state = READ;
    data->fd = client_fd;
    std::memset(data->buf, 0, sizeof(128));
    struct io_uring_sqe *sqe = io_uring_get_sqe(&m_cRing);
    io_uring_prep_read(sqe, client_fd, data->buf, 128, 0);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&m_cRing);
}
void TcpChannel::startWrite(int client_fd)
{
    ConnectionData *data = m_umConnections[client_fd];
    data->state = WRITE;
    data->fd = client_fd;
    struct io_uring_sqe *sqe = io_uring_get_sqe(&m_cRing);

    //设置回送数据
    std::string message="received: "+std::string(data->buf);
    //setEchoMessage(data,message);

    io_uring_prep_write(sqe, client_fd,data->buf, sizeof(data->buf), 0);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&m_cRing);

    //清空缓冲区
    memset(data->buf,0,sizeof(data->buf)/sizeof(char));
}
void TcpChannel::setEchoMessage(ConnectionData *data,const std::string str)
{
    strcpy(data->buf,str.c_str());
}
void TcpChannel::processEvent(io_uring_cqe *cqe)
{
    //带外数据
    ConnectionData *data = (ConnectionData *)cqe->user_data;
    int clientfd;
    //std::cout<<"state:"<<data->state<<std::endl;
    //判断带外数据类型
    switch (data->state)
    {
    case ACCEPT:
        if (cqe->res > 0)
        {
            // 客户端文件描述符
            clientfd = cqe->res;
            processAccept(clientfd);
        }
        //std::cout << "accept fd:"<<cqe->res << std::endl;
        break;

    case READ:
        clientfd = data->fd;
        if (cqe->res > 0)
        {
            std::cout << "receive data:"<<data->buf << std::endl;
            startWrite(clientfd);

        }
        else
        {
            clear(clientfd);
        }
        //std::cout << "read res:"<<cqe->res << std::endl;
        break;
    case WRITE:
        clientfd = data->fd;
        if (cqe->res > 0)
        {
            startRead(clientfd);
        }
        else
        { 
            clear(clientfd);
        }
        //std::cout << "write res:"<<cqe->res << std::endl;
        break;
    default:
        std::cout << "error " << std::endl;
        break;
    }
}
//关闭描述符
void TcpChannel::clear(int client_fd)
{
    close(client_fd);
    std::cout<<"close"<<client_fd<<std::endl;
    //回收内存
    delete m_umConnections[client_fd];
    m_umConnections.erase(client_fd);
}
// 构造函数，根据ip和端口号得到socket描述符
TcpChannel::TcpChannel(const std::string ip, const int port, int thread_num) : m_sIp(ip), m_nPort(port), m_cThreadPool(thread_num), m_cSocketData(new ConnectionData)
{
    m_nSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_nSocketFd < 0)
    {
        perror("creat socket faile\n");
        exit(1);
    }
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_nPort);
    addr.sin_addr.s_addr = inet_addr(m_sIp.c_str());
    if (bind(m_nSocketFd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind fail");
        exit(1);
    }
    if (listen(m_nSocketFd, 128) < 0)
    {
        perror("listen fail\n");
        exit(1);
    }
    std::cout <<"socket_fd:"<<m_nSocketFd << std::endl;
    if (io_uring_queue_init(8192, &m_cRing, 0) != 0)
    {
        std::cerr<<"queue init error."<<std::endl;
        exit(-1);
    }
    m_cSocketData->fd = m_nSocketFd;
    m_cSocketData->state = ACCEPT;
    m_cSocketData->addr.lens = sizeof(sockaddr_in);
}

TcpChannel::~TcpChannel()
{
    close(m_nSocketFd);
}