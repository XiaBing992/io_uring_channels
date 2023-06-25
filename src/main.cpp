#include "tcpchannel.h"
#include "filechannel.h"


void FileTest()
{
    FileChannel fileChannel("./test.txt");
    fileChannel.write("qwertyuisdafsghjkuykuukukuukuozcxxzvvbnmb,");

}
void ServerTest()
{
    TcpChannel tcpChannel{"127.0.0.1",3737,8};
    tcpChannel.run();
}
int main()
{
    ServerTest();
    return 0;
}