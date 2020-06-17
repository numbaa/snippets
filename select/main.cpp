#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#else
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <iostream>

int serve()
{
    return 0;
} 

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << "<port>" << std::endl;
        return -1;
    }
    int port = std::atoi(argv[1]);
    if (port < 1000 || port > 65535)
    {
        std::cerr << "Port should be an integer between [1001, 65535]" << std::endl;
        return -1;
    }
    return serve();
}