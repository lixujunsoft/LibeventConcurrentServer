#include <event2/event.h>
#include <iostream>
using namespace std;

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
	if (features & EV_FEATURE_ET) {
		cout << "EV_FEATURE_ET" << endl;
	} 

	if (features & EV_FEATURE_EARLY_CLOSE) {
		cout << "EV_FEATURE_EARLY_CLOSE" << endl;
	}

	if (features & EV_FEATURE_FDS) {
		cout << "EV_FEATURE_FDS" << endl;
	}

	if (features & EV_FEATURE_O1) {
		cout << "EV_FEATURE_O1" << endl;
	}
}

int main()
{
#ifdef _WIN32 
	//初始化socket库
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#endif
	// 创建配置上下文
	event_config *conf = event_config_new();

	// 如果设置了EV_FEATURE_FDS，其他特征就无法设置, 此特征不支持epoll
	// event_config_require_features(conf, EV_FEATURE_FDS);
	// event_config_require_features(conf, EV_FEATURE_ET | EV_FEATURE_EARLY_CLOSE | EV_FEATURE_O1);
	
	// 设置网络模型使用select
	// event_config_avoid_method(conf, "epoll");
	// event_config_avoid_method(conf, "poll");
	
	// 根据配置项 conf 初始化libevent上下文
	event_base *base = event_base_new_with_config(conf);
	event_config_free(conf);
	if (NULL == base) {
		cout << "NULL point: base, event_base_new_with_config failed!" << endl;
		exit(-1);
	}

	printSupprotedMethods(base);
	printSupportedFeatures(base);
	event_base_free(base);

	return 0;
}
