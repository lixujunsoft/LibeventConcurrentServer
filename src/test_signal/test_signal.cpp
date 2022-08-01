#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>
using namespace std;

#define SPORT 5001

// sock文件描述符，which 事件类型 arg 传递的参数
static void Ctrl_C(int sock, short which, void *arg)
{
	cout << "Ctrl_C" << endl;
}

static void Kill_15(int sock, short which, void *arg)
{
	cout << "Kill_15" << endl;
	event *ev = (event *)arg;
	// exit(-1);
	// 如果处于非待诀
	if (!evsignal_pending(ev, NULL))
	{
		event_del(ev);
		event_add(ev, NULL);
	}
}

int main()
{

	event_base *base = event_base_new();
	// 添加ctrl + c 信号事件，处于no pending状态
	event *csig = evsignal_new(base, SIGINT, Ctrl_C, base);
	if (NULL == csig) {
		cerr << "SIGINT evsignal_new failed!" << endl;
		return -1;
	}

	// 添加事件到pending
	if (event_add(csig, 0) != 0) {
		cerr << "SIGINT evsignal_new failed" << endl;
		return -1;
	}

	// 添加kill信号
	// 非持久事件，只进入一次 event_self_cbarg() 传递当前的event
	event *ksig = event_new(base, SIGTERM, EV_SIGNAL, Kill_15, event_self_cbarg());
	if (NULL == ksig) {
		cerr << "SIGTERM evsignal_new failed!" << endl;
		return -1;
	}

	if (event_add(ksig, 0) != 0) {
		cerr << "SIGTERM event_add_failed!" << endl;
		return -1;
	}

	// 进入事件主循环
	event_base_dispatch(base);
	event_free(csig);
	event_base_free(base);

	return 0;
}
