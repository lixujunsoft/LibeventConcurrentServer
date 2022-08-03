#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <string>
using namespace std;

#define SPORT 5001

enum bufferevent_filter_result filter_in(struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
    enum bufferevent_flush_mode mode, void *ctx) 
{
	cout << "filter_in" << endl;
	// 接收客户端发送的文件名
	char data[1024] = {0};
	int len = evbuffer_remove(src, data, sizeof(data));
	evbuffer_add(dst, data, len);

	return BEV_OK;
}

void read_cb(bufferevent *bev, void *arg)
{
	cout << "******read_cb*******" << endl;
	// 回复OK
	char data[1024] = {0};
	bufferevent_read(bev, data, sizeof(data));
	cout << data << flush;
	bufferevent_write(bev, "OK", 3);
}

void event_cb(bufferevent *bev, short events, void *arg)
{
	cout << "event_cb" << endl;
}

void listen_cb(struct evconnlistener *e, evutil_socket_t s, struct sockaddr *addr, int socklen, void * arg)
{
	cout << "listen_cb" << endl;
	event_base *base = (event_base*)arg;
	// 1 创建一个bufferevent用来通信
	bufferevent *bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);
	// 2 添加过滤，并设置输入回调
	bufferevent *bev_filter = bufferevent_filter_new(bev,
		filter_in,  // 输入过滤函数
		NULL, // 输出过滤函数
		BEV_OPT_CLOSE_ON_FREE,  // 关闭bev_filter时同时关闭bev
		NULL,       // 清理的回调函数
		0);         // 传递给回调函数的参数
	// 3 设置回调 读取 事件（处理连接断开）
	bufferevent_setcb(bev_filter, read_cb, NULL, event_cb, NULL);
	bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}

int main()
{
	//创建libevent的上下文
	event_base *base = event_base_new();
	if (NULL == base) {
		cout << "NULL point: base" << endl;
		exit(-1);
	}
	
	// 监听端口
	// socket, bind, listen 绑定事件
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);

	evconnlistener *ev = evconnlistener_new_bind(base,  // libevent的上下文
		listen_cb,                 // 接收到连接的上下文
		base,                      // 回调函数获取的参数
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, // 地址重用，evconnlistener关闭同时关闭socket
		10,                        // 连接队列大小（对应listen函数）
		(sockaddr*)&sin,
		sizeof(sin)
	);

	if (NULL == ev) {
		cout << "NULL point: ev" << endl;
		exit(-1);
	}

	// 事件分发处理
	event_base_dispatch(base);
	evconnlistener_free(ev);
	event_base_free(base);
	return 0;
}
