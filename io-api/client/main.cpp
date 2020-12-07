#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cstring>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage:" << argv[0] << " <port>" << std::endl;
        return 1;
    }
    int port = std::atoi(argv[1]);
    if (port <= 1000 || port > 65535) {
        std::cerr << "port must be an integer in [1001, 65535]";
        return 1;
    }
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Create socket failed, errno:" << errno
                  << ", error:" << std::strerror(errno) << std::endl;
        return 1;
    }
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
    server_addr.sin_port = ::htons(uint16_t { port });
    int ret = ::connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof server_addr);
    if (ret < 0) {
        std::cerr << "Connect to 127.0.0.1:" << port << " failed, errno:"
                  << errno << ", error:" << std::strerror(errno) << std::endl;
        return 1;
    }

    char buff[2048] = { 0 };
    int input = fileno(stdin);
    fd_set fds;
    FD_SET(sock, &fds);
    FD_SET(input, &fds);
    while (true) {
        int ret = ::select(sock + 1, &fds, nullptr, nullptr, nullptr);
        if (ret < 0) {
            std::cerr << "select() failed, errno:" << errno
                      << ", error:" << std::strerror(errno) << std::endl;
            return 1;
        }
        if (FD_ISSET(sock, &fds)) {
            int bytes = ::read(sock, buff, 2048);
            if (bytes <= 0) {
                std::cout << "Disconnect" << std::endl;
                return 0;
            }
            std::cout << "Received:" << std::string(buff, bytes) << std::endl;
        }
        if (FD_ISSET(input, &fds)) {
            int bytes = ::read(input, buff, 2048);
            if (bytes <= 0) {
                std::cout << "Stop" << std::endl;
                return 0;
            }
            ::write(sock, buff, bytes);
        }
        FD_SET(sock, &fds);
        FD_SET(input, &fds);
    }
    return 0;
}