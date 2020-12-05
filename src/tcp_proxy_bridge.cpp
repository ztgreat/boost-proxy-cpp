#include <cstddef>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <tcp_proxy_bridge.hpp>

namespace proxy {
    namespace tcp_proxy {
        namespace ip = boost::asio::ip;

        proxy::tcp_proxy::bridge::bridge(boost::asio::io_service &ios)
                : downstream_socket_(ios),
                  upstream_socket_(ios) {}

        void proxy::tcp_proxy::bridge::start(const std::string &upstream_host, unsigned short upstream_port) {
            // Attempt connection to remote server (upstream side)
            upstream_socket_.async_connect(
                    ip::tcp::endpoint(
                            boost::asio::ip::address::from_string(upstream_host),
                            upstream_port),
                    boost::bind(&bridge::handle_upstream_connect,
                                shared_from_this(),
                                boost::asio::placeholders::error));
        }

        void proxy::tcp_proxy::bridge::handle_upstream_connect(const boost::system::error_code &error) {
            if (!error) {
                // Setup async read from remote server (upstream)
                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_, max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

                // Setup async read from client (downstream)
                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_, max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            } else
                close();
        }


        void proxy::tcp_proxy::bridge::handle_upstream_read(const boost::system::error_code &error,
                                                            const size_t &bytes_transferred) {
            if (!error) {
                async_write(downstream_socket_,
                            boost::asio::buffer(upstream_data_, bytes_transferred),
                            boost::bind(&bridge::handle_downstream_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
            } else
                close();
        }

        void proxy::tcp_proxy::bridge::handle_downstream_write(const boost::system::error_code &error) {
            if (!error) {
                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_, max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            } else
                close();
        }

        void proxy::tcp_proxy::bridge::handle_downstream_read(const boost::system::error_code &error,
                                                              const size_t &bytes_transferred) {
            if (!error) {
                async_write(upstream_socket_,
                            boost::asio::buffer(downstream_data_, bytes_transferred),
                            boost::bind(&bridge::handle_upstream_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
            } else
                close();
        }

        void proxy::tcp_proxy::bridge::handle_upstream_write(const boost::system::error_code &error) {
            if (!error) {
                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_, max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            } else
                close();
        }

        void proxy::tcp_proxy::bridge::close() {
            boost::mutex::scoped_lock lock(mutex_);

            if (downstream_socket_.is_open()) {
                downstream_socket_.close();
            }

            if (upstream_socket_.is_open()) {
                upstream_socket_.close();
            }
        }

    }
}