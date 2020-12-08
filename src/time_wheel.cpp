
#include <string>
#include <vector>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include "time_wheel.hpp"

namespace proxy {

    namespace timer {

        time_wheel::time_wheel() {
            cur_slot = 0;
            for (int i = 0; i < N; i++) {
                slots[i] = nullptr;
            }
        }

        time_wheel::~time_wheel() {
            /* 遍历每个槽，并销毁其中的定时器 */
            for (int i = 0; i < N; i++) {
                /* 释放链表上的每个节点 */
                tw_timer *tmp = slots[i];
                while (tmp) {
                    slots[i] = tmp->next;
                    delete tmp;
                    tmp = slots[i];
                }
            }
        }

        tw_timer *time_wheel::add_timer(int timeout) {
            if (timeout < 0) {
                return nullptr;
            }
            int ticks = 0;
            /* 下面根据带插入定时器的超时值计算它将在时间轮转动多少个滴答后被触发，并将该滴答
             * 数存储于变量ticks中。如果待插入定时器的超时值小于时间轮的槽间隔SI，则将ticks
             * 向上折合为1，否则就将ticks向下折合为timeout/SI */
            if (timeout < SI) {
                ticks = 1;
            } else {
                ticks = timeout / SI;
            }
            /* 计算待插入的定时器在时间轮转动多少圈后被触发 */
            int rotation = ticks / N;
            /* 计算待插入的定时器应该被插入到哪个槽中 */
            int ts = (cur_slot + (ticks % N)) % N;
            /* 创建新的定时器，它在时间轮转动ratation圈之后被触发，且位于第ts个槽上 */
            tw_timer *timer = new tw_timer(rotation, ts);
            /* 如果第ts个槽中无任何定时器，则把新建的定时器插入其中，并将该定时器设置为该槽的头结点 */
            if (slots[ts] == nullptr) {
                printf("add timer, rotation is %d,cur_slot is %d\n", rotation, ts, cur_slot);
                slots[ts] = timer;
            } else {
                /* 头插法在链表中插入节点 */
                timer->next = slots[ts];
                slots[ts]->prev = timer;

                slots[ts] = timer;
            }
            return timer;
        }

        void time_wheel::del_timer(tw_timer *timer) {
            if (timer == nullptr) {
                return;
            }
            int ts = timer->time_slot;
            /* slots[ts]是目标定时器所在槽的头结点。如果目标定时器就是该头结点，则需要
             * 重置第ts个槽的头结点 */
            if (timer == slots[ts]) {
                slots[ts] = slots[ts]->next;
                if (slots[ts]) {
                    slots[ts]->prev = nullptr;
                }
                delete timer;
            } else {
                timer->prev->next = timer->next;
                if (timer->next) {
                    timer->next->prev = timer->prev;
                }
                delete timer;
            }
        }

        void time_wheel::tick() {
            tw_timer *tmp = slots[cur_slot];    /* 取得时间轮上当前槽的头结点 */
            printf("current slots is %d\n", cur_slot);
            while (tmp) {
                printf("tick the timer once\n");
                /* 如果定时器的ratation值大于0，则它在这一轮中不起作用 */
                if (tmp->rotation > 0) {
                    tmp->rotation--;
                    tmp = tmp->next;
                }
                    /* 否则说明定时器已经到期，于是执行定时任务，然后删除该定时器 */
                else {
                    tmp->cb_func(tmp->user_data);
                    if (tmp == slots[cur_slot]) {
                        printf("delete header in cur_slot\n");
                        slots[cur_slot] = tmp->next;
                        delete tmp;
                        if (slots[cur_slot]) {
                            slots[cur_slot]->prev = nullptr;
                        }
                        tmp = slots[cur_slot];
                    } else {
                        tmp->prev->next = tmp->next;
                        if (tmp->next) {
                            tmp->next->prev = tmp->prev;
                        }
                        tw_timer *tmp2 = tmp->next;
                        delete tmp;
                        tmp = tmp2;
                    }
                }
            }
            /* 更新时间轮的当前槽，以反映时间轮的转动 */
            cur_slot = cur_slot + 1;
            cur_slot = cur_slot % N;
        }
    }
}