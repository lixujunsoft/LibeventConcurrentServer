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
	char data[1024] = {0};
	// 读取并清理原数据
	int len = evbuffer_remove(src, data, sizeof(data) - 1);

	// 将所有字母转化成大写
	for (int i = 0; i < len; i++) {
		data[i] = toupper(data[i]);
	} 

	evbuffer_add(dst, data, len);
	return BEV_OK;
}

enum bufferevent_filter_result filter_out(struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
    enum bufferevent_flush_mode mode, void *ctx) 
{
	cout << "filter_out" << endl;

	char data[1024] = {0};
	// 读取并清理原数据
	int len = evbuffer_remove(src, data, sizeof(data) - 1);

	string str = "";
	str += "===============\n";
	str += data;
	str += "===============\n";

	evbuffer_add(dst, str.c_str(), str.size());
	return BEV_OK;
}

void read_cb(bufferevent *bev, void *arg)
{
	cout << "read_cb" << endl;
	char data[1024] = {0};
	int len = bufferevent_read(bev, data, sizeof(data) - 1);
	cout << data << endl;

	// 回复客户消息，经过输出过滤器
	bufferevent_write(bev, data, len);
}

void write_cb(bufferevent *bev, void *arg)
{
	cout << "write_cb" << endl;
}

void event_cb(bufferevent *bev, short events, void *arg)
{
	cout << "event_cb" << endl;
}

void listen_cb(struct evconnlistener *e, evutil_socket_t s, struct sockaddr *addr, int socklen, void * arg)
{
	cout << "listen_cb" << endl;
	// 创建bufferevent 绑定bufferevent filter
	bufferevent *bev = bufferevent_socket_new((event_base*)arg, s, BEV_OPT_CLOSE_ON_FREE);
	bufferevent *bev_filter = bufferevent_filter_new(bev, 
		filter_in,              // 输入过滤函数
		filter_out,             // 输出过滤函数
		BEV_OPT_CLOSE_ON_FREE,
		NULL,                      // 清理的回调函数
		0);                        // 传递给回调的参数

	// 设置bufferevent的回调
	bufferevent_setcb(bev_filter, read_cb, write_cb, event_cb, NULL);
	bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}

int main()
{
#ifdef _WIN32 
	//初始化socket库
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#else
	// 忽略管道信号，发送给已关闭的socket
	if (signal(SIGPIPE, SIG_IGN)) {
		return 1;
	}
#endif

    std::cout << "test libevent!\n"; 
	//创建libevent的上下文
	event_base *base = event_base_new();
	if (NULL == base) {
		cout << "NULL point: base" << endl;
		exit(-1);
	}
	
	cout << "event_base_new success!" << endl;

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
