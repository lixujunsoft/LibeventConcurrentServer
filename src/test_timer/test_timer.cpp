#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>
using namespace std;

static timeval t1 = {1, 0};

void timer1(int sock, short which, void *arg)
{
	cout << "[timer1]" << endl;
	event *ev = (event*)arg;
	if (!evtimer_pending(ev, &t1)) {
		evtimer_del(ev);
		evtimer_add(ev, &t1);
	}
}

void timer2(int sock, short which, void *arg)
{
	cout << "[timer2]" << endl;
}

void timer3(int sock, short which, void *arg)
{
	cout << "[timer3]" << endl;
}

#define SPORT 5001

int main()
{
	event_base *base = event_base_new();
	
	// 定时器 非持久事件(只进入一次)
	event *ev1 = evtimer_new(base, timer1, event_self_cbarg());
	if (NULL == ev1) {
		cout << "evtimer_new timer1 failed!" << endl;
		return -1;
	}

	evtimer_add(ev1, &t1);

	timeval tv2;
	tv2.tv_sec = 0;
	tv2.tv_usec = 200000;

	event *ev2 = event_new(base, -1, EV_PERSIST, timer2, 0);
	event_add(ev2, &tv2);

	// 超时性能优化，默认event用二插堆存储(完全二插树) 插入删除O(logn)
    // 优化到双向队列，插入删除O(1)
	event *ev3 = event_new(base, -1, EV_PERSIST, timer3, 0);
	static timeval tv_in = {3, 0};
	const timeval *tv3;
	tv3 = event_base_init_common_timeout(base, &tv_in);
	event_add(ev3, tv3);  // 插入性能O（1） 

	event_base_dispatch(base);
	event_base_free(base);

	return 0;
}
