#include <pthread.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <multi_process.hpp>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <algorithm>
#include <iostream>

namespace proxy {

    namespace process {
        std::vector<int> multi_process::signals = {SIGHUP, SIGTERM, SIGINT, SIGQUIT, SIGPIPE, SIGUSR1, SIGUSR2};
        int multi_process::sig = -1;
        std::vector<std::pair<pid_t, int>> multi_process::pids;

        void proxy::process::multi_process::signal_cb(int sig) {
            multi_process::sig = sig;
            for (auto &i : multi_process::pids) {
                if (i.first > 0) {
                    kill(i.first, sig);
                }
            }
        }

        void proxy::process::multi_process::set_signal() {
            std::vector<int> sigs = multi_process::signals;
            for (size_t i = 0; i < sigs.size(); ++i) {
                signal(sigs[i], multi_process::signal_cb);
            }
        }

        proxy::process::multi_process::multi_process()
                : mtx(0), mtx_attr(0), data(0) {
            this->mtx = (pthread_mutex_t *) mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE,
                                                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            if (this->mtx != MAP_FAILED) {
                this->mtx_attr = (pthread_mutexattr_t *) mmap(0, sizeof(pthread_mutexattr_t), PROT_READ | PROT_WRITE,
                                                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
                if (this->mtx_attr != MAP_FAILED) {

                    pthread_mutexattr_init(this->mtx_attr);
                    pthread_mutexattr_setpshared(this->mtx_attr, PTHREAD_PROCESS_SHARED);
                    pthread_mutexattr_settype(this->mtx_attr, PTHREAD_MUTEX_DEFAULT);
                    pthread_mutex_init(this->mtx, this->mtx_attr);
                }

                this->data = (size_t *) mmap(0, sizeof(size_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1,
                                             0);
                if (this->data != MAP_FAILED) {
                    pthread_mutex_lock(this->mtx);
                    *this->data = 0;
                    pthread_mutex_unlock(this->mtx);
                }
            }
        }

        proxy::process::multi_process::~multi_process() {
            if (this->mtx != MAP_FAILED) {
                pthread_mutex_destroy(this->mtx);
                munmap(this->mtx, sizeof(pthread_mutex_t));

                if (this->mtx_attr != MAP_FAILED) {
                    pthread_mutexattr_destroy(this->mtx_attr);
                    munmap(this->mtx_attr, sizeof(pthread_mutexattr_t));
                }
                if (this->data != MAP_FAILED) {
                    munmap(this->data, sizeof(size_t));
                }
            }
        }


        pid_t proxy::process::multi_process::forker(int len, const std::function<void()> &f,
                                                    std::vector<std::pair<pid_t, int>> &pids) {
            pid_t pid = fork();
            if (pid == 0) {
                f();
            } else if (pid > 0) {
                pids.push_back({pid, -1});
                if (len > 1) {
                    forker(len - 1, f, pids);
                }
                return pid;
            } else {
                perror("fork error.");
            }
            return -1;
        }

        bool proxy::process::multi_process::process_bind_cpu(pid_t pid, int cpu) {
            cpu_set_t set;
            CPU_ZERO(&set);
            CPU_SET(cpu, &set);
            return sched_setaffinity(pid, sizeof(cpu_set_t), &set) == 0;
        }

        void proxy::process::multi_process::run(const std::function<void(pthread_mutex_t *, size_t *)> &function,
                                                const std::function<bool(int)> &g,
                                                size_t process_size) {
            std::function<void()> process_work = [&]() {
                prctl(PR_SET_NAME, std::to_string(getppid()).append(":worker").c_str());
                function(this->mtx, this->data);
            };
            multi_process::forker((process_size > 0 ? process_size : std::thread::hardware_concurrency()), process_work,
                                  multi_process::pids);
            multi_process::set_signal();
            for (size_t i = 0; i < multi_process::pids.size(); ++i) {
                if (multi_process::process_bind_cpu(multi_process::pids[i].first, i)) {
                    multi_process::pids[i].second = i;
                }
            }

            std::function<void(pid_t)> refork = [&](pid_t pid) {
                if (multi_process::forker(1, process_work, multi_process::pids) > 0) {
                    std::vector<std::pair<pid_t, int>>::iterator p = std::find_if(multi_process::pids.begin(),
                                                                                  multi_process::pids.end(),
                                                                                  [=](const std::pair<pid_t, int> &item) {
                                                                                      return item.first == pid;
                                                                                  });
                    if (p != multi_process::pids.end()) {
                        multi_process::pids.back().second = p->second;
                        multi_process::process_bind_cpu(multi_process::pids.back().first, p->second);
                        p->second = -1;
                        p->first = -1 * pid;
                    }
                }
            };
            pid_t pid;
            int status;
            while ((pid = wait(&status)) > 0) {
                if (WIFEXITED(status) && multi_process::sig == SIGHUP) {
                    refork(pid);
                }
                if (WIFSIGNALED(status)) {
                    if (WCOREDUMP(status)) {
                        if (g(status)) {
                            refork(pid);
                        }
                    } else if(multi_process::sig != SIGINT) {
                          refork(pid);
                    }
                }
            }
        }
    }
}
