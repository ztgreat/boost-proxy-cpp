#ifndef PROXY_MULTI_PROCESS_HPP
#define PROXY_MULTI_PROCESS_HPP
#include <functional>
namespace proxy {
    namespace process {
        class multi_process {
        public:
            static std::vector<int> signals;

        public:
            multi_process();

            virtual ~multi_process();

            void run(const std::function<void(pthread_mutex_t *, size_t *)> &f, const std::function<bool(int)> &g,
                     size_t = std::thread::hardware_concurrency());

            pid_t forker(int, const std::function<void()> &, std::vector<std::pair<pid_t, int>> &);

            bool process_bind_cpu(pid_t pid, int cpu);

        private:
            pthread_mutex_t *mtx;
            pthread_mutexattr_t *mtx_attr;
            size_t *data;
            static int sig;
            static std::vector<std::pair<pid_t, int>> pids;

            static void signal_cb(int sig);

            static void set_signal();
        };
    }
}
#endif
