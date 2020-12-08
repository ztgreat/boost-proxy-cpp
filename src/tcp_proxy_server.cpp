#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <tcp_proxy_server.hpp>
#include <multi_process.hpp>

namespace proxy {
    namespace tcp_proxy {
        namespace ip = boost::asio::ip;

        std::atomic_bool proxy::tcp_proxy::server::done(true);

        proxy::tcp_proxy::server::server(boost::asio::io_service &io_service,
                                         const std::string &local_host, unsigned short local_port,
                                         size_t back_log = 4096)
                : io_service_(io_service),
                  localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
                  backlog(back_log),
                  acceptor_(io_service_) {
            acceptor_.open(boost::asio::ip::tcp::v4());
            int one = 1;
            sid = 1;
            setsockopt(this->acceptor_.native_handle(), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));
            setsockopt(this->acceptor_.native_handle(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
            acceptor_.bind(ip::tcp::endpoint(localhost_address, local_port));
            this->acceptor_.listen(backlog);
        }

        void proxy::tcp_proxy::server::set_route_locators(std::vector<route_locator *> &route_locator) {
            this->route_locators = &route_locator;
        }

        void proxy::tcp_proxy::server::signal_normal_cb(int sig, siginfo_t *, void *) {
            proxy::tcp_proxy::server::done = false;
        }

        void proxy::tcp_proxy::server::run() {
            accept_connections();
            io_service_.run();
        }

        bool proxy::tcp_proxy::server::accept_connections() {
            try {
                session_ = boost::shared_ptr<tcp_proxy::server::bridge>(new bridge(io_service_, *this));
                acceptor_.async_accept(session_->downstream_socket_,
                                       boost::bind(&server::handle_accept,
                                                   this,
                                                   boost::asio::placeholders::error));
            }
            catch (std::exception &e) {
                std::cerr << "acceptor exception: " << e.what() << std::endl;
                return false;
            }
            return true;
        }

        void proxy::tcp_proxy::server::handle_accept(const boost::system::error_code &error) {
            if (!error) {
                again:
                if (this->sid_queue.empty()) {
                    session_->sid = ++this->sid;
                } else {
                    session_->sid = this->sid_queue.front();
                    this->sid_queue.pop();
                }
                auto it = this->clients.find(session_->sid);
                proxy::tcp_proxy::server::bridge::ptr_type client = nullptr;
                if (it != this->clients.end()) {
                    client = it->second;
                }
                if (client != nullptr) {
                    //session_->upstream_socket_ = std::move(client->upstream_socket());
                    client->sid = session_->sid;
                    client->downstream_socket_ = std::move(session_->downstream_socket_);
                    session_ = client;
                    this->clients[session_->sid] = session_;
                    if (session_->start()) {
                        goto again;
                    }

                } else {
                    proxy::upstream_server *upstreamServer = nullptr;
                    for (auto route : *(this->route_locators)) {
                        upstreamServer = route->choseServer(nullptr);
                        if (upstreamServer) {
                            break;
                        }
                    }
                    if (upstreamServer == nullptr) {
                        session_->downstream_socket().close();
                        return;
                    }
                    session_->start(upstreamServer->server, upstreamServer->port);
                    //this->clients[session_->sid] = session_;
                    //this->sid_queue.push(session_->sid);
                }
                if (!accept_connections()) {
                    std::cerr << "accept fail" << std::endl;
                }
            } else {
                std::cerr << "error: " << error.message() << std::endl;
            }
        }

        proxy::tcp_proxy::server::bridge::bridge(boost::asio::io_service &ios,
                                                 proxy::tcp_proxy::server &server_ptr)
                : downstream_socket_(ios),
                  upstream_socket_(ios), server(server_ptr) {}

        void proxy::tcp_proxy::server::bridge::start(const std::string &upstream_host, unsigned short upstream_port) {
            upstream_socket_.async_connect(
                    ip::tcp::endpoint(
                            boost::asio::ip::address::from_string(upstream_host),
                            upstream_port),
                    boost::bind(&bridge::handle_upstream_connect,
                                shared_from_this(),
                                boost::asio::placeholders::error));
        }

        bool proxy::tcp_proxy::server::bridge::start() {

            if (!upstream_socket_.is_open()) {
                return false;
            }

            upstream_socket_.async_read_some(
                    boost::asio::buffer(upstream_data_, max_data_length),
                    boost::bind(&bridge::handle_upstream_read,
                                shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));

            downstream_socket_.async_read_some(
                    boost::asio::buffer(downstream_data_, max_data_length),
                    boost::bind(&bridge::handle_downstream_read,
                                shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));

            return true;
        }


        void proxy::tcp_proxy::server::bridge::handle_upstream_connect(const boost::system::error_code &error) {
            if (!error) {
                boost::asio::ip::tcp::no_delay no_delay(true);
                upstream_socket_.set_option(no_delay);
                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_, max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_, max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

            } else
                close_all();
        }


        void proxy::tcp_proxy::server::bridge::handle_upstream_read(const boost::system::error_code &error,
                                                                    const size_t &bytes_transferred) {
            if (!error) {
                async_write(downstream_socket_,
                            boost::asio::buffer(upstream_data_, bytes_transferred),
                            boost::bind(&bridge::handle_downstream_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
            } else {

                close_all();
            }
        }

        void proxy::tcp_proxy::server::bridge::handle_downstream_write(const boost::system::error_code &error) {
            if (!error) {
                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_, max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            } else {
                std::cout << error.message() << std::endl;
                close_all();
            }

        }

        void proxy::tcp_proxy::server::bridge::handle_downstream_read(const boost::system::error_code &error,
                                                                      const size_t &bytes_transferred) {
            if (!error) {
                async_write(upstream_socket_,
                            boost::asio::buffer(downstream_data_, bytes_transferred),
                            boost::bind(&bridge::handle_upstream_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
            } else
                close_downstream();
        }

        void proxy::tcp_proxy::server::bridge::handle_upstream_write(const boost::system::error_code &error) {
            if (!error) {
                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_, max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                                    shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));
            } else {
                close_all();
            }
        }

        void proxy::tcp_proxy::server::bridge::close_all() {
            boost::mutex::scoped_lock lock(mutex_);

            if (downstream_socket_.is_open()) {
                downstream_socket_.close();
                auto it = this->server.clients.find(this->sid);
                if (it == this->server.clients.end()) {
                    this->server.clients[this->sid] = shared_from_this();
                    this->server.sid_queue.push(this->sid);
                }
            }

            if (upstream_socket_.is_open()) {
                upstream_socket_.close();
                this->server.clients.erase(this->sid);
            }
        }

        void proxy::tcp_proxy::server::bridge::close_downstream() {
            boost::mutex::scoped_lock lock(mutex_);
            if (downstream_socket_.is_open()) {
                downstream_socket_.close();
                auto it = this->server.clients.find(this->sid);
                if (it == this->server.clients.end()) {
                    this->server.clients[this->sid] = shared_from_this();
                    this->server.sid_queue.push(this->sid);
                }
            }
        }

    }
}