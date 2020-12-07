#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
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
    int reuse = 1;
    int ret = ::setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret < 0) {
        std::cerr << "setsockopt(REUSEADDR) error: " << ::strerror(errno) << std::endl;
        return -1;
    }
    int opts = ::fcntl(server_socket, F_GETFL);
    if (opts < 0) {
        std::cerr << "fcntl(F_GETFL) error: " << ::strerror(errno) << std::endl;
        return -1;
    }
    ret = ::fcntl(server_socket, F_SETFL, opts | O_NONBLOCK);
    if (ret < 0) {
        std::cerr << "fcntl(F_SETFL) error: " << ::strerror(errno) << std::endl;
        return -1;
    }
    ret = ::bind(server_socket, reinterpret_cast<sockaddr*>(&listen_addr), sizeof(listen_addr));
    if (ret < 0) {
        std::cerr << "bind() error: " << ::strerror(errno) << std::endl;
        return -1;
    }
    ret = ::listen(server_socket, 3);
    if (ret < 0) {
        std::cerr << "bind() error: " << ::strerror(errno) << std::endl;
        return -1;
    }

    int epfd = ::epoll_create(1024);
    if (epfd < 0) {
        std::cerr << "epoll_create() error:" << ::strerror(errno) << std::endl;
        return -1;
    }
    ::epoll_event ev;
    ev.data.fd = server_socket;
    ev.events = EPOLLIN | EPOLLET;
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &ev);
    std::array<::epoll_event, 1024> epevs;
    while (true) {
        int nums = ::epoll_wait(epfd, epevs.data(), epevs.size(), -1);
        if (nums < 0) {
            int error_code = errno;
            std::cerr << "epoll_wait() error with errno:"
                      << error_code << " " << std::strerror(error_code);
            std::exit(-1);
        }
        // for (int i = 0; i < nums; i++) {
        //     if (!(epevs[i].events | EPOLLIN))
        //         continue;
        //     if (epevs[i].data.fd == server_socket) {
        //         std::cout << "start accept\n";
        //         while (true) {
        //             int new_socket = ::accept(server_socket, nullptr, nullptr);
        //             if (new_socket < 0) {
        //                 int error_code = errno;
        //                 if (error_code == EAGAIN)
        //                     break;
        //                 std::cerr << "accept() error with errno:"
        //                           << error_code << " " << std::strerror(error_code);
        //                 std::exit(-1);
        //             }
        //             std::cout << "accept " << new_socket << std::endl;
        //             ::epoll_event ev;
        //             ev.data.fd = new_socket;
        //             ev.events = EPOLLIN | EPOLLET;
        //             ::epoll_ctl(epfd, EPOLL_CTL_ADD, new_socket, &ev);
        //         }
        //     } else {
        //         int bytes_read = ::read(epevs[i].data.fd, buffer.data(), buffer.size());
        //         ::write(epevs[i].data.fd, buffer.data(), bytes_read);
        //     }
        // }
        for (int i = 0; i < nums; i++) {
            if (!(epevs[i].events | EPOLLIN))
                continue;
            if (epevs[i].data.fd == server_socket) {
                std::cout << "start accept\n";
                //漏？
                int new_socket = ::accept(server_socket, nullptr, nullptr);
                if (new_socket < 0) {
                    int error_code = errno;
                    if (error_code == EAGAIN)
                        continue;
                    std::cerr << "accept() error with errno:"
                              << error_code << " " << std::strerror(error_code);
                    std::exit(-1);
                }
                std::cout << "accept " << new_socket << std::endl;
                ::epoll_event ev;
                ev.data.fd = new_socket;
                ev.events = EPOLLIN | EPOLLET;
                ::epoll_ctl(epfd, EPOLL_CTL_ADD, new_socket, &ev);
            } else {
                int bytes_read = ::read(epevs[i].data.fd, buffer.data(), buffer.size());
                ::write(epevs[i].data.fd, buffer.data(), bytes_read);
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

/**
 * epoll 写proactor，需要附着在某个executor上。调用async_read(..., handler)时将闭包[::read(xxx)]进队。成功时i回调handler。失败时将闭包添加进epoll_ctl(add)的参数里。
 */