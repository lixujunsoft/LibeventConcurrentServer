#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <errno.h>
using namespace std;

#define SPORT 5001

// 正常的断开连接会进入，超时也会进入
void client_cb(evutil_socket_t s, short w, void *arg)
{
	// 水平触发LT，只要有数据没有处理，会一直进入
	// 边缘触发ET,需要进行读数据处理（read_n）
#if 0
	cout << "client_cb" << endl;
	return;
#endif
	// 判断超时
	if (w & EV_TIMEOUT) {
		cout << "timeout" << endl;
		event_free((event*)arg);
		evutil_closesocket(s);
		return;
	}

	char buf[1024] = {0};
	int len = recv(s, buf, sizeof(buf) -1, 0);
	if (len > 0) {
		cout << buf;
		send(s, "OK", 2, 0);
	} else {
		cout << "event_free" << endl;
		event_free((event*)arg);
		evutil_closesocket(s);
	}
}

void listen_cb(evutil_socket_t s, short w, void *arg)
{
	cout << "listen_cb" << endl;
	// 读取连接信息
	sockaddr_in sin;
	socklen_t size = sizeof(sin);
	evutil_socket_t client = accept(s, (sockaddr*)&sin, &size);
	char ip[16] = {0};
	evutil_inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip));
	cout << "client ip is " << ip << endl;
	
	// 客户端数据读取事件
	// event *ev = event_new((event_base*)arg, client, EV_READ | EV_PERSIST, client_cb, event_self_cbarg());
	event *ev = event_new((event_base*)arg, client, EV_READ | EV_PERSIST | EV_ET, client_cb, event_self_cbarg());
	timeval t = {10, 0};
	event_add(ev, &t);
}

void printSupprotedMethods(event_base *base)
{
	/*
	const char **methods = event_get_supported_methods();
	cout << "Current System Supported Methods******" << endl;
	for (; *methods != NULL; methods++) {
		cout << *methods << endl;
	}
	*/

	const char *currentMethods = event_base_get_method(base);
	cout << "Current Methods is " << currentMethods << endl;
}

void printSupportedFeatures(event_base *base)
{
	int features = event_base_get_features(base);

	cout << "Supported Features******" << endl;
	if (features & EV_FEATURE_ET)
	{
		cout << "EV_FEATURE_ET" << endl;
	}

	if (features & EV_FEATURE_EARLY_CLOSE)
	{
		cout << "EV_FEATURE_EARLY_CLOSE" << endl;
	}

	if (features & EV_FEATURE_FDS)
	{
		cout << "EV_FEATURE_FDS" << endl;
	}

	if (features & EV_FEATURE_O1)
	{
		cout << "EV_FEATURE_O1" << endl;
	}
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

	//创建libevent的上下文
	event_base *base = event_base_new();
	cout << "test event server" << endl;

	evutil_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock <= 0) {
		cout << "socket error: " << strerror(errno) << endl;
		return -1;
	}

	// 设置地址复用和非阻塞
	evutil_make_socket_nonblocking(sock);
	evutil_make_listen_socket_reuseable(sock);

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);
	int ret = ::bind(sock, (sockaddr*)&sin, sizeof(sin));
	if (ret != 0) {
		cerr << "bind error:" << strerror(errno) << endl;
		return -1;
	}

	listen(sock, 10);

	// 开始接受连接事件,默认是水平触发
	event *ev = event_new(base, sock, EV_READ | EV_PERSIST, listen_cb, base);
	event_add(ev, 0);

	printSupprotedMethods(base);
	printSupportedFeatures(base);

	event_base_dispatch(base);

	evutil_closesocket(sock);
	event_base_free(base);
	return 0;
}
