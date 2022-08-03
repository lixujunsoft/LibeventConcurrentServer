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
#define FILE_PATH "001.txt"

enum bufferevent_filter_result filter_out(struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
    enum bufferevent_flush_mode mode, void *ctx) 
{
	cout << "filter_out" << endl;
	char data[1024] = {0};
	int len = evbuffer_remove(src, data, sizeof(data));
	evbuffer_add(dst, data, len);
	return BEV_OK;
}

static void read_cb(bufferevent *bev, void *arg)
{
	cout << "read_cb" << endl;
	char data[1024] = {0};
	bufferevent_read(bev, data, sizeof(data) - 1);
	if (strcmp(data, "OK") == 0) {
		cout << data << endl;
		// 触发发送事件,触发写入回调
		bufferevent_trigger(bev, EV_WRITE, 0);
	} else {
		bufferevent_free(bev);
	}
}

static void write_cb(bufferevent *bev, void *arg)
{
	cout << "write_cb" << endl;
	FILE *fp = (FILE*)arg;
	// 读取文件
	char data[1024] = {0};
	int len = fread(data, 1, sizeof(data), fp);
	if (len < 0) {
		fclose(fp);
		// bufferevent_free(bev); // 立刻清理
		bufferevent_disable(bev, EV_WRITE);
		return;
	}
	// 发送文件
	bufferevent_write(bev, data, len);
}

void event_cb(bufferevent *bev, short events, void *arg)
{
	cout << "event_cb" << endl;
	if (events & BEV_EVENT_CONNECTED) {
		cout << "BEV_EVENT_CONNECTED" << endl;
		bufferevent_write(bev, FILE_PATH, strlen(FILE_PATH));
		// 创建过滤器
		bufferevent *bev_filter = bufferevent_filter_new(bev, NULL, filter_out, BEV_OPT_CLOSE_ON_FREE, NULL, NULL);
		FILE *fp = fopen(FILE_PATH, "rb");
		if (!fp) {
			cout << "open file " << FILE_PATH << "failed" << endl;
			return;
		}
		// 设置读取、写入和事件的回调
		bufferevent_setcb(bev_filter, read_cb, write_cb, event_cb, fp);
		bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
	}
}

int main()
{
	//创建libevent的上下文
	event_base *base = event_base_new();
	if (NULL == base) {
		cout << "NULL point: base" << endl;
		exit(-1);
	}
	
	// 创建socket
	bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);
	evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);

	// 只绑定连接事件回调，用来确认连接成功
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	bufferevent_setcb(bev, NULL, NULL, event_cb, NULL);

	bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));
	// 事件分发处理
	event_base_dispatch(base);
	event_base_free(base);
	return 0;
}
