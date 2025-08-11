# Message
一个及时回调的消息派发器，单头文件引用，简单方便

## 使用方法

```C++
// 包含头文件
#include "Message.h"

// 实例化一个派发器
Message::Dispatcher dispatcher;

// 定义消息
struct Custom {
	int value;
};

// 添加监听
dispatcher.AddListener<Custom>(this, [](auto& msg){
	// 使用参数
	std::cout << msg.value << std::endl;
});
// 删除监听
dispatcher.RemoveListener<Custom>(this);

// 发送消息
Custom custom_msg;
custom_msg.value = 1000;
dispatcher.Send(custom_msg);
```

```C++
// 支持任意结构的消息
int int_msg { 10 };
dispatcher.Send(int_msg);

std::string str_msg { "hello" };
dispatcher.Send(str_msg);
```
