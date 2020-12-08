
#include <string>
#include <vector>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#ifndef TIMER_WHEEL_TIME_WHEEL_HPP
#define TIMER_WHEEL_TIME_WHEEL_HPP

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include "tw_timer.hpp"

namespace proxy {

    namespace timer {

        class time_wheel {
        public:
            time_wheel();

            ~time_wheel();

            /* 根据定时值timeout创建一个定时器，并把它插入到合适的槽中 */
            tw_timer *add_timer(int timeout);

            /* 删除目标定时器timer */
            void del_timer(tw_timer *timer);

            /* SI时间到后，调用该函数，时间轮向前滚动一个槽的间隔 */
            void tick();

        private:
            static const int N = 60;    /* 时间轮上槽的数量 */
            static const int SI = 1;    /* 每1 s时间轮转动一次，即槽间隔为1 s */
            tw_timer *slots[N];         /* 时间轮的槽，其中每个元素指向一个定时器链表，链表无序 */
            int cur_slot;               /* 时间轮的当前槽 */
        };

    }
}
#endif //TIMER_WHEEL_TIME_WHEEL_H
