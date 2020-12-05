
#include <string>
#include "upstream_server.hpp"
#include <vector>
#include "route_locator.hpp"
#include "request.hpp"

namespace proxy {

    proxy::upstream_server *proxy::route_locator::choseServer(const proxy::http::request *request) {

        // tcp
        if (request == nullptr) {
            return this->load_balance->choseServer();
        }

        // http
        // todo need optimization
        std::vector<std::string> param;
        for (auto it = this->route_predicate->begin(); it != this->route_predicate->end(); it++) {
            if ((*it)->match(*request, param)) {
                proxy::upstream_server *up = this->load_balance->choseServer();
                return up;
            }
        }
        return nullptr;

    }

}
