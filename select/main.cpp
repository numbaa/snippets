#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#else
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <set>

// class Proactor {
// public:
//     void read(socket_t fd, std::function<void(const std::error_code& code, std::size_t bytes_transferred)> handler);
//     void write(socket_t fd, std::function<void()> handler);
// };

int serve(uint16_t port)
{
    std::array<uint8_t, 2048> buffer {};

    sockaddr_in listen_addr;
    ::memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    listen_addr.sin_port = ::htons(port);
    int server_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    int ret = ::bind(server_socket, reinterpret_cast<sockaddr*>(&listen_addr), sizeof(listen_addr));
    if (ret < 0) {
        std::cerr << "bind() error: " << ::strerror(errno) << std::endl;
        return -1;
    }
    ret = ::listen(server_socket, 5);
    if (ret < 0) {
        std::cerr << "bind() error: " << ::strerror(errno) << std::endl;
        return -1;
    }

    fd_set readfds_tpl;
    std::set<int> fds;
    int max_fd = server_socket;
    FD_SET(server_socket, &readfds_tpl);
    while (true) {
        fd_set readfds = readfds_tpl;
        int nums = ::select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
        if (nums < 0) {
            int error_code = errno;
            std::cerr << "select() error with errno:"
                      << error_code << " " << std::strerror(error_code);
            std::exit(-1);
        }
        if (FD_ISSET(server_socket, &readfds)) {
            //do_accept()
            //accept几次？
            int new_socket = ::accept(server_socket, nullptr, nullptr);
            if (new_socket < 0) {
                int error_code = errno;
                std::cerr << "accept() error with errno:"
                          << error_code << " " << std::strerror(error_code);
                std::exit(-1);
            }
            FD_SET(new_socket, &readfds_tpl);
            fds.insert(new_socket);
            max_fd = new_socket > max_fd ? new_socket : max_fd;
            nums--;
        }
        auto iter = fds.cbegin();
        while (nums != 0 && iter != fds.cend()) {
            if (FD_ISSET(*iter, &readfds)) {
                nums--;
                int bytes_read = ::read(*iter, buffer.data(), buffer.size());
                ::write(*iter, buffer.data(), bytes_read);
            }
            ++iter;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << "<port>" << std::endl;
        return -1;
    }
    int port = std::atoi(argv[1]);
    if (port < 1000 || port > 65535) {
        std::cerr << "Port should be an integer between [1001, 65535]" << std::endl;
        return -1;
    }
    return serve(port);
}