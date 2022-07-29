#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>
using namespace std;

#define SPORT 5001

void listen_cb(struct evconnlistener *e, evutil_socket_t s, struct sockaddr *addr, int socklen, void * arg)
{
	cout << "listen_cb" << endl;
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
