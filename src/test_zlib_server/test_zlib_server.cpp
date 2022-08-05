#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>
using namespace std;

#define SPORT 5001

static int fd;
static bool flag1 = true;
static bool flag2 = true;

static z_stream *z_intput;

enum bufferevent_filter_result filter_in(struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
    enum bufferevent_flush_mode mode, void *ctx) 
{
	if (flag1) {
		// 接收客户端发送的文件名
		char data[1024] = {0};
		int len = evbuffer_remove(src, data, sizeof(data));
		evbuffer_add(dst, data, len);
		cout << data << endl;
		flag1 = false;
		return BEV_OK;
	}

	// 解压
	evbuffer_iovec v_in[1];
	// 读取数据，不清理缓存
	int n = evbuffer_peek(src, -1, NULL, v_in, 1);
	if (n <= 0) {
		return BEV_NEED_MORE;
	}
	
	// 输入数据大小
	z_intput->avail_in = v_in[0].iov_len;
	z_intput->next_in = (Byte *)v_in[0].iov_base;

	// 申请输出空间大小，输出空间地址
	evbuffer_iovec v_out[1];
	evbuffer_reserve_space(dst, 1024 * 64, v_out, 1);

	// zlib 输出空间大小
	z_intput->avail_out = v_out[0].iov_len;
	z_intput->next_out = (Byte *)v_out[0].iov_base;

	// 解压数据
	int re = inflate(z_intput, Z_SYNC_FLUSH);
	if (Z_OK != re) {
		cout << "inflate failed" << endl;
	}

	// 解压用了多少数据, 从source evbuffer中移除
	// z_output->avail_in 传入传出参数：
	// 解压用掉的数据 = 输入数据 - 剩余数据 
	int nread = v_in[0].iov_len - z_intput->avail_in; 

	// 压缩后的数据大小，传入des evbuffer
	int nwrite = v_out[0].iov_len - z_intput->avail_out;

	// 移除source evbuffer中的数据
	evbuffer_drain(src, nread);

	// 传入des evbuffer
	v_out[0].iov_len = nwrite;
	evbuffer_commit_space(dst, v_out, 1);

	cout << "iov_len = " << v_in[0].iov_len << "nread = " << nread << endl;    // 读取的数据
	cout << "nwrite = " << nwrite << endl;  // 压缩过后的数据
	return BEV_OK;
}

void read_cb(bufferevent *bev, void *arg)
{
	cout << "******read_cb*******" << endl;
	char data[1024] = {0};
	int len;

    // 一定要将数据读完
	while ((len = bufferevent_read(bev, data, sizeof(data))) > 0) {
		if (flag2) {
			fd = open(data, O_RDWR | O_CREAT | O_TRUNC, 0664);
			if (fd < 0) {
				cout << "open failed: fd = " << fd << endl;
				exit(0);
			}
			bufferevent_write(bev, "OK", 3);
			flag2 = false;
			memset(data, 0, sizeof(data));
			continue;
		}

		if (!flag2) {
			write(fd, data, len);
		}
		cout << len << endl;
		memset(data, 0, sizeof(data));
	}
}

void event_cb(bufferevent *bev, short events, void *arg)
{
	cout << "event_cb" << endl;
	if (events & BEV_EVENT_EOF) {
		cout << "BEV_EVENT_EOF" << endl;
		fsync(fd);
		close(fd);
	}
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

	z_intput = new z_stream();
	inflateInit(z_intput);

	bufferevent_setcb(bev_filter, read_cb, NULL, event_cb, NULL);
	bufferevent_enable(bev_filter, EV_READ | EV_WRITE | 0666);
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
