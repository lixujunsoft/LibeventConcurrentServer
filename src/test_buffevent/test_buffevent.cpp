#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <iostream>
#include <signal.h>
#include <string.h>
using namespace std;

#define SPORT 5001

// 错误、超时、连接断开时回调用
void event_cb(bufferevent *be, short events, void *arg)
{
	// cout << "[E]" << flush;
	if (events & BEV_EVENT_TIMEOUT && events & BEV_EVENT_READING) {
		cout << "BEV_EVENT_TIMEOUT" << endl;
		// bufferevent_enable(be, EV_READ);
		bufferevent_free(be);
	} else if (events & BEV_EVENT_ERROR) {
		bufferevent_free(be);
	} else {
		cout << "OTHERS" << endl;
	}
}

void write_cb(bufferevent *be, void *arg)
{
	// cout << "[W]" << flush;
}

void read_cb(bufferevent *be, void *arg)
{
	// cout << "[R]" << flush;
	char data[1024] = {0};

	// 读取输入缓冲的数据
	int len = bufferevent_read(be, data, sizeof(data) - 1);
	cout << data << flush;
	if (len <= 0) {
		return;
	}

	// 发送数据，写入到输出缓冲
	bufferevent_write(be, "OK", 3); 
}

void listen_cb(evconnlistener *ev, evutil_socket_t s, sockaddr *sin, int slen, void *arg)
{
	cout << "listen_cb" << endl;

	// 创建bufferevent上下文, BEV_OPT_CLOSE_ON_FREE清理bufferevent时关闭socket
	bufferevent *bev = bufferevent_socket_new((event_base*)arg, s, BEV_OPT_CLOSE_ON_FREE);
	
	// 添加监控事件
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	
	// 设置水位
	// 读取水位
	bufferevent_setwatermark(bev, EV_READ, 0, 0);
	// bufferevent_setwatermark(bev, EV_WRITE, 5, 0); 

	// 超时时间的设置
	timeval tv = {3, 0};
	bufferevent_set_timeouts(bev, &tv, 0);
	
	// 设置回调函数
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, (event_base*)arg);
}

void test1()
{
	event_base *base = event_base_new();
    // 创建网络服务器
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);

	evconnlistener *ev = evconnlistener_new_bind(base, listen_cb,
		base,  // 传递给回调函数的参数
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,  // 地址重用 | 关闭ev时同步关闭创建的socket
		10,                                         // listen 的backlog大小
		(sockaddr*)&sin,
		sizeof(sin)
		);
	event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
	event_base_free(base);
}

int main()
{
	test1();

	return 0;
}