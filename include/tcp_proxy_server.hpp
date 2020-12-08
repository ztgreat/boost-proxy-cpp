#ifndef PROXY_TCP_PROXY_SERVER_HPP
#define PROXY_TCP_PROXY_SERVER_HPP

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include "route_locator.hpp"
#include <queue>
#include <list>

namespace proxy {
    namespace tcp_proxy {
        namespace ip = boost::asio::ip;
        typedef ip::tcp::socket socket_type;

        class server{
        public:

            class bridge : public boost::enable_shared_from_this<bridge> {
            public:

                size_t sid;

                typedef boost::shared_ptr<bridge> ptr_type;

                socket_type downstream_socket_;

                socket_type upstream_socket_;

                bridge(boost::asio::io_service &ios,proxy::tcp_proxy::server &);

                socket_type &downstream_socket() {
                    // client socket
                    return downstream_socket_;
                }

                socket_type &upstream_socket() {
                    // up server socket
                    return upstream_socket_;
                }

                void start(const std::string &upstream_host, unsigned short upstream_port);


                bool start();

                void handle_upstream_connect(const boost::system::error_code &error);

            private:

                void handle_upstream_read(const boost::system::error_code &error,
                                          const size_t &bytes_transferred);

                void handle_downstream_write(const boost::system::error_code &error);

                void handle_downstream_read(const boost::system::error_code &error,
                                            const size_t &bytes_transferred);

                void handle_upstream_write(const boost::system::error_code &error);

                void close_all();

                void close_downstream();


                enum {
                    //暂时设置 1500
                    max_data_length = 1500
                };
                unsigned char downstream_data_[max_data_length];
                unsigned char upstream_data_[max_data_length];
                boost::mutex mutex_;
                proxy::tcp_proxy::server &server;

            };
        public:
            std::unordered_map<int, proxy::tcp_proxy::server::bridge::ptr_type> clients;

            server(boost::asio::io_service &io_service,
                   const std::string &local_host, unsigned short local_port, size_t);

            bool accept_connections();

            void run();

            static void signal_normal_cb(int sig, siginfo_t *, void *);

            void set_route_locators(std::vector<route_locator *> &);

            ~server() {
                for (auto it = this->route_locators->begin(); it != this->route_locators->end(); it++) {
                    delete *it;
                }
                this->route_locators->clear();
                this->route_locators->shrink_to_fit();
                delete this->route_locators;
            }

        private:
            void handle_accept(const boost::system::error_code &error);

            static std::atomic_bool done;
            boost::asio::io_service &io_service_;
            ip::address_v4 localhost_address;
            size_t backlog;
            ip::tcp::acceptor acceptor_;
            std::vector<route_locator *> *route_locators;
            proxy::tcp_proxy::server::bridge::ptr_type session_;
            std::queue<size_t, std::list<size_t>> sid_queue;
            size_t sid;
        };

    }
}
#endif
