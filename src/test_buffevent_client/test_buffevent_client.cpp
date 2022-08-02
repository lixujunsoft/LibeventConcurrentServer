#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <iostream>
#include <signal.h>
#include <string.h>
using namespace std;

#define SPORT 5001

// 错误、超时、连接断开时回调用
void client_event_cb(bufferevent *be, short events, void *arg)
{
	cout << "[E]" << flush;
	if (events & BEV_EVENT_TIMEOUT && events & BEV_EVENT_READING) {
		cout << "BEV_EVENT_TIMEOUT" << endl;
		// bufferevent_enable(be, EV_READ);
		bufferevent_free(be);
	} else if (events & BEV_EVENT_ERROR) {
		bufferevent_free(be);
		return;
	} else if (events & BEV_EVENT_EOF) {
		cout << "sever closed connect" << endl;
		bufferevent_free(be);
		return;
	}

	if (events & BEV_EVENT_CONNECTED) {
		cout << "BEV_EVENT_CONNECTED" << endl;
		// 触发write
		bufferevent_trigger(be, EV_WRITE, 0);
	}
}

void client_write_cb(bufferevent *be, void *arg)
{
	cout << "[W]" << flush;
	FILE *fp = (FILE*)arg;
	char data[1024] = {0};
	int len = fread(data, 1, sizeof(data - 1), fp);
	if (len <= 0) { // 读到结尾或者文件出错
		fclose(fp);
		// bufferevent_free(be); // 立刻清理，可能会造成缓冲数据没有发送结束
		bufferevent_disable(be, EV_WRITE);
		return;
	}
	// 写入buffer
	bufferevent_write(be, data, len);
}

void client_read_cb(bufferevent *be, void *arg)
{
	cout << "[R]" << flush;
}

void test1()
{
	event_base *base = event_base_new();

	// 调用客户端代码
	// -1 内部创建socket
	bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(5001);
	evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);
	
	FILE *fp = fopen("test_buffevent_client.cpp", "rb");

	// 设置回调函数
	bufferevent_setcb(bev, client_read_cb, client_write_cb, client_event_cb, fp);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	int re = bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));
	if (re == 0) {
		cout << "connected:" << strerror(errno) << endl;
	}

	event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
	event_base_free(base);
}

int main()
{
	test1();

	return 0;
}