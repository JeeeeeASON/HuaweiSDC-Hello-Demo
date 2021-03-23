/**
 * hello Demo是以事件（event）为驱动的APP，event.h中声明了所有对APP事件进行处理相关的结构体、函数。
 */

#ifndef APP_EVENT_H
#define APP_EVENT_H

#include <stdint.h>
#include <stddef.h>

#include <sys/epoll.h>

#ifdef __cplusplus
extern "C"{
#endif 

/**
 * [container_of ]
 */
#define container_of(addr, type, member) ((type*)((char*)(addr) - offsetof(type, member)))

struct app_ctx;
struct app_event;

/**
 * [pf_app_event_handle  指向某一事件的句柄的函数指针]
 * @param  	event 	[指向句柄所属的事件的指针]
 *          events 	[传递给epoll其中events表示感兴趣的事件和被触发的事件，可能的取值为：
 *          			EPOLLIN：表示对应的文件描述符可以读；
 *          			EPOLLOUT：表示对应的文件描述符可以写；
 *          			EPOLLPRI：表示对应的文件描述符有紧急的数可读；
 *          			EPOLLERR：表示对应的文件描述符发生错误；
 *          			EPOLLHUP：表示对应的文件描述符被挂断；
 *          			EPOLLET： ET的epoll工作模式；
 *         ]
 *         app_ctx 	[指向app运行环境的指针]
 * @return  [none]
 */
typedef void (*pf_app_event_handle)(struct app_event* event, uint32_t events, struct app_ctx* app_ctx);

/**
 * 整个APP的核心，app的事件结构体，每一种事件都由一个时间结构体记录，包含两个成员：
 * 	1.操作该事件的句柄handle
 *  2.该事件的文件描述符fd
 */
struct app_event {
    pf_app_event_handle handle;
    int fd;
};

/**
 * 下列是各种对于app事件的操作函数
 */
void app_event_init(struct app_event* app_event, int fd, pf_app_event_handle handle);

int app_event_add(struct app_event* event, uint32_t events, struct app_ctx* app_ctx);

int app_event_mod(struct app_event* event, uint32_t events, struct app_ctx* app_ctx);

void app_event_del(struct app_event* event, struct app_ctx* app_ctx);

void app_event_del_close(struct app_event* event, struct app_ctx* app_ctx);

struct app_timer;


/**
 * [void  pf_app_timer_handle	app计时器的句柄]
 * @param  app_ctx [description]
 * @return         [description]
 */
typedef void (*pf_app_timer_handle)(struct app_timer* timer, struct app_ctx* app_ctx);

/**
 * app的计时器，有三个成员，
 * 1、base	某一个app的事件
 */
struct app_timer {
    struct app_event base;
    pf_app_timer_handle handle;
    int interval;
};

/**
 * 下列是各种对于app计时器的操作
 */
void app_timer_init(struct app_timer* timer, pf_app_timer_handle handle);

int app_timer_add(struct app_timer* timer, uint32_t timeout, uint32_t interval, struct app_ctx* app_ctx);

void app_timer_del(struct app_timer* timer, struct app_ctx* app_ctx);

#ifdef __cplusplus
}
#endif


#endif

