#ifndef PROXY_LROUTE_LOCATOR_HPP
#define PROXY_LROUTE_LOCATOR_HPP

#include <string>
#include <upstream_server.hpp>
#include <vector>
#include "request.hpp"
#include "route_predicate.hpp"
#include "load_balance.hpp"

namespace proxy {
    class route_locator {
    public:
        std::vector<proxy::route_predicate *> *route_predicate;
        proxy::load_balance *load_balance;
    public:
        route_locator() {
            this->route_predicate = new std::vector<proxy::route_predicate *>();
        };

        ~route_locator() {
            for (auto it = this->route_predicate->begin(); it != this->route_predicate->end(); it++) {
                delete *it;
            }
            this->route_predicate->clear();
            this->route_predicate->shrink_to_fit();
            delete this->route_predicate;
        }

        void addPredicate(proxy::route_predicate *predicate) {
            this->route_predicate->insert(this->route_predicate->end(), predicate);
        }

        void setLoadBalance(proxy::load_balance *loadBalance) {
            this->load_balance = loadBalance;
        }

        virtual upstream_server *choseServer(const proxy::http::request *);
    };
}
#endif
