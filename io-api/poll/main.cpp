#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <set>
#include <vector>

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

    std::vector<::pollfd> pfds_tpl { ::pollfd { server_socket, POLLIN, 0 } };
    while (true) {
        auto pfds = pfds_tpl;
        int nums = ::poll(pfds.data(), pfds.size(), -1);
        if (nums < 0) {
            int error_code = errno;
            std::cerr << "poll() error with errno:"
                      << error_code << " " << std::strerror(error_code);
            std::exit(-1);
        }
        if (pfds[0].revents & POLLIN) {
            //do_accept()
            //accept几次？
            int new_socket = ::accept(server_socket, nullptr, nullptr);
            if (new_socket < 0) {
                int error_code = errno;
                std::cerr << "accept() error with errno:"
                          << error_code << " " << std::strerror(error_code);
                std::exit(-1);
            }
            nums--;
            pfds_tpl.push_back(::pollfd { new_socket, POLLIN, 0 });
        }

        for (int i = 1; i < pfds.size() && nums > 0; i++) {
            if (pfds[i].revents & POLLIN) {
                int bytes_read = ::read(pfds[i].fd, buffer.data(), buffer.size());
                ::write(pfds[i].fd, buffer.data(), bytes_read);
                nums--;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return -1;
    }
    int port = std::atoi(argv[1]);
    if (port < 1000 || port > 65535) {
        std::cerr << "Port should be an integer between [1001, 65535]" << std::endl;
        return -1;
    }
    return serve(port);
}