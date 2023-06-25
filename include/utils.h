enum STATE
{
    READ,
    WRITE,
    ACCEPT,
};
struct ConnectionData
{
    int fd;
    STATE state;
    union
    {
        struct
        {
            sockaddr_in ipv4_addr;
            socklen_t lens;
        } addr;
        char buf[128];
    };
};
struct conninfo
{
    int connfd;
    int type;
};