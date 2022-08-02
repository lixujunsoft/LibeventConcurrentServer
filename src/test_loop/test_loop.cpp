#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>
using namespace std;

#define SPORT 5001
// #define TEST2
static bool isexit = false;


// sock文件描述符，which 事件类型 arg 传递的参数
static void Ctrl_C(int sock, short which, void *arg)
{
	cout << "Ctrl_C" << endl;
#ifdef TEST2
	isexit = true;
#endif
	// 执行完当前处理的事件函数就退出
	// event_base_loopbreak((event_base*)arg);

	// 运行完所有的活动事件再退出
	// 事件循环没有运行时，也要等运行一次再退出
	timeval t = {3, 0};  // 在上面的情况都满足的情况下，至少运行3秒钟退出
	event_base_loopexit((event_base*)arg, &t);
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

void test1()
{
	event_base *base = event_base_new();
	// 添加ctrl + c 信号事件，处于no pending状态
	event *csig = evsignal_new(base, SIGINT, Ctrl_C, base);
	if (NULL == csig) {
		cerr << "SIGINT evsignal_new failed!" << endl;
		return;
	}

	// 添加事件到pending
	if (event_add(csig, 0) != 0) {
		cerr << "SIGINT evsignal_new failed" << endl;
		return;
	}

	// 添加kill信号
	// 非持久事件，只进入一次 event_self_cbarg() 传递当前的event
	event *ksig = event_new(base, SIGTERM, EV_SIGNAL, Kill_15, event_self_cbarg());
	if (NULL == ksig) {
		cerr << "SIGTERM evsignal_new failed!" << endl;
		return;
	}

	if (event_add(ksig, 0) != 0) {
		cerr << "SIGTERM event_add_failed!" << endl;
		return;
	}

	// EVLOOP_ONCE 等待一个事件运行，直到没有活动事件就退出
	event_base_loop(base, EVLOOP_ONCE);

	event_free(csig);
	event_base_free(base);
}

void test2()
{
	event_base *base = event_base_new();
	// 添加ctrl + c 信号事件，处于no pending状态
	event *csig = evsignal_new(base, SIGINT, Ctrl_C, base);
	if (NULL == csig) {
		cerr << "SIGINT evsignal_new failed!" << endl;
		return;
	}

	// 添加事件到pending
	if (event_add(csig, 0) != 0) {
		cerr << "SIGINT evsignal_new failed" << endl;
		return;
	}

	// 添加kill信号
	// 非持久事件，只进入一次 event_self_cbarg() 传递当前的event
	event *ksig = event_new(base, SIGTERM, EV_SIGNAL, Kill_15, event_self_cbarg());
	if (NULL == ksig) {
		cerr << "SIGTERM evsignal_new failed!" << endl;
		return;
	}

	if (event_add(ksig, 0) != 0) {
		cerr << "SIGTERM event_add_failed!" << endl;
		return;
	}

	// EVLOOP_NONBLOCK 有活动事件就处理，没有就返回
	while (!isexit) {
		event_base_loop(base, EVLOOP_NONBLOCK);
	}

	event_free(csig);
	event_base_free(base);
}

void test3()
{
	event_base *base = event_base_new();

#if 1
	// 添加ctrl + c 信号事件，处于no pending状态
	event *csig = evsignal_new(base, SIGINT, Ctrl_C, base);
	if (NULL == csig) {
		cerr << "SIGINT evsignal_new failed!" << endl;
		return;
	}

	// 添加事件到pending
	if (event_add(csig, 0) != 0) {
		cerr << "SIGINT evsignal_new failed" << endl;
		return;
	}
#endif

#if 1
	// 添加kill信号
	// 非持久事件，只进入一次 event_self_cbarg() 传递当前的event
	event *ksig = event_new(base, SIGTERM, EV_SIGNAL, Kill_15, event_self_cbarg());
	if (NULL == ksig) {
		cerr << "SIGTERM evsignal_new failed!" << endl;
		return;
	}

	if (event_add(ksig, 0) != 0) {
		cerr << "SIGTERM event_add_failed!" << endl;
		return;
	}
#endif
	// EVLOOP_NO_EXIT_ON_EMPTY 没有注册事件也不返回，用于事件后期添加
	// event_base_loop(base, EVLOOP_ONCE);
	event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

	// event_free(csig);
	event_base_free(base);
}

int main()
{
	// test1(); // EVLOOP_ONCE 等待一个事件运行，直到没有活动事件就退出
	// test2(); // EVLOOP_NONBLOCK 有活动事件就处理，没有就返回
	test3(); // EVLOOP_NO_EXIT_ON_EMPTY 没有注册事件也不返回，用于事件后期添加

	return 0;
}