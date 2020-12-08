#include <string>
#include <vector>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

namespace proxy {

    namespace timer {

#ifndef TIMER_WHEEL_TW_TIMER_HPP
#define TIMER_WHEEL_TW_TIMER_HPP


#define BUFFER_SIZE 64

        class tw_timer;

        /* 绑定socket和定时器 */
        struct client_data {
            sockaddr_in address;
            int sockfd;
            char buf[BUFFER_SIZE];
            tw_timer *timer;
        };

        /* 定时器类 */
        class tw_timer {
        public:
            tw_timer(int rot, int ts) {
                next = nullptr;
                prev = nullptr;
                rotation = rot;
                time_slot = ts;
            }

        public:
            int rotation;                       /* 记录定时器在时间轮转多少圈后生效 */
            int time_slot;                      /* 记录定时器属于时间轮上的哪个槽(对应的链表，下同) */
            void (*cb_func)(client_data *);     /* 定时器回调函数 */
            client_data *user_data;             /* 客户端数据 */
            tw_timer *next;                     /* 指向下一个定时器 */
            tw_timer *prev;                     /* 指向前一个定时器 */
        };

    }
}
#endif
