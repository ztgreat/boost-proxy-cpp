#ifndef PROXY_HTTP_REQUEST_HPP
#define PROXY_HTTP_REQUEST_HPP

#include <string>
#include <unordered_map>

namespace proxy {
    namespace http {
        class request {
        public:
            request()
                    : client(), user_agent(), method(), uri(), param(), headers(), form(), cookies(), session(),
                      cache() {
            }

            virtual ~request() = default;

            void clean() {
                this->client.clear();
                this->user_agent.clear();
                this->method.clear();
                this->uri.clear();
                this->param.clear();

                this->headers.clear();
                this->form.clear();
                this->cookies.clear();
                this->session.clear();
                this->cache.clear();
            }

            std::string client, user_agent, method, uri, param;
            std::unordered_map<std::string, std::string> headers, form, cookies, session, cache;
        };
    }
}

#endif
