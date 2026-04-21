# my_Muduo

一个基于 C++11 实现的轻量级 Reactor 网络库，参考 muduo 的核心设计思想，聚焦于 Linux 高并发网络编程中最重要的几个模块：`EventLoop`、`Channel`、`Poller`、`TcpServer`、`TcpConnection`、线程池与 Buffer。

这个项目适合作为学习版 muduo、C++ 网络编程练手项目，也可以作为理解 Reactor 模型、epoll、非阻塞 IO、多线程事件分发的代码样本。

## Highlights

- 基于 `epoll` 的 Reactor 事件驱动模型
- 支持非阻塞 TCP server
- 主从 Reactor 线程模型
- 封装 `EventLoop`、`Channel`、`Poller`、`Acceptor`、`TcpConnection`
- 支持连接回调、消息回调、写完成回调、高水位回调
- 使用 `eventfd` 实现跨线程唤醒
- 内置动态 Buffer，支持 `readv` 分散读
- 提供 echo server 示例，方便快速验证
- 代码规模小，结构清晰，适合阅读和二次开发

## Architecture

```text
TcpServer
   |
   v
Acceptor ---- accepts new fd ----> TcpConnection
   |                                  |
   v                                  v
EventLoopThreadPool              Channel
   |                                  |
   v                                  v
EventLoop ---------------------> Poller(epoll)
```

核心流程：

1. `TcpServer` 持有 `Acceptor`，负责监听端口。
2. `Acceptor` 收到新连接后创建 `TcpConnection`。
3. 每个连接绑定到某个 `EventLoop`，由 `Channel` 管理 fd 事件。
4. `Poller` 基于 `epoll_wait` 获取活跃事件。
5. `TcpConnection` 在读事件中读取数据并触发用户设置的 message callback。

## Directory

```text
.
├── Acceptor.*              # 监听 socket，接收新连接
├── Buffer.*                # 应用层输入/输出缓冲区
├── Channel.*               # fd 与事件回调的封装
├── EPollPoller.*           # epoll 实现
├── EventLoop.*             # 事件循环
├── EventLoopThread.*       # one loop per thread
├── EventLoopThreadPool.*   # IO 线程池
├── InetAddress.*           # 地址封装
├── Socket.*                # socket 操作封装
├── TcpConnection.*         # TCP 连接抽象
├── TcpServer.*             # TCP server 封装
├── Thread.*                # 线程封装
├── Logger.*                # 简单日志
└── examples/
    └── testserver.cc       # echo server 示例
```

## Build

项目依赖 Linux 网络 API，例如 `epoll`、`eventfd`、`accept4`，建议在 Linux 或 WSL/Linux 虚拟机中构建。

```bash
mkdir -p build
cd build
cmake .. -DMYMUDUO_BUILD_EXAMPLES=ON
make
```

构建完成后会生成动态库：

```text
lib/libmymuduo.so
```

如果开启 `MYMUDUO_BUILD_EXAMPLES`，还会生成测试程序：

```text
build/testserver
```

## Quick Start

启动 echo server：

```bash
./testserver
```

另开一个终端连接：

```bash
nc 127.0.0.1 8000
```

输入任意内容，服务端会原样回显。

如果没有 `nc`，也可以使用：

```bash
telnet 127.0.0.1 8000
```

## Example

```cpp
#include "TcpServer.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Logger.h"

#include <string>

void onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        LOG_INFO("new connection: %s\n", conn->name().c_str());
    }
    else
    {
        LOG_INFO("connection closed: %s\n", conn->name().c_str());
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime)
{
    std::string msg = buffer->retrieveAllAsString();
    conn->send(msg);
}

int main()
{
    EventLoop loop;
    InetAddress listenAddr(8000, "0.0.0.0");
    TcpServer server(&loop, listenAddr, "EchoServer");

    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setThreadNum(3);
    server.start();

    loop.loop();
    return 0;
}
```

完整示例见 [examples/testserver.cc](examples/testserver.cc)。

## Why This Project

很多网络库的源码对初学者来说比较庞大，直接阅读会被大量工程细节打断。`my_Muduo` 保留了 muduo 最经典、最值得学习的主线：

- Reactor 如何组织事件循环
- `epoll` 如何与业务回调解耦
- TCP 连接生命周期如何管理
- 主线程与 IO 线程如何分工
- 跨线程任务如何安全投递
- Buffer 如何处理半包场景的基础读写缓存

你可以从 `examples/testserver.cc` 开始运行，再沿着 `TcpServer -> Acceptor -> TcpConnection -> Channel -> EventLoop -> EPollPoller` 的顺序阅读源码。

## Recent Fixes

- 修复 `EventLoop::hasChannel()` 缺少返回值的问题。
- 修复 `Acceptor` 忽略 `reuseport` 参数的问题。
- 修复跨线程发送字符串时可能出现悬空指针的问题。
- 补齐 `TcpConnection::send(const void*, int)` 实现。
- 增加默认 callback，避免未设置回调时触发空函数调用。
- 增加 echo server 示例和 CMake 示例构建选项。

## Environment

推荐环境：

- Linux kernel 3.9+
- CMake 2.5+
- GCC/G++ with C++11 support
- pthread

注意：当前代码使用了 Linux 专有系统调用，不支持直接在原生 Windows 环境编译。

## Roadmap

- 增加单元测试
- 增加定时器模块
- 增加 HTTP server 示例
- 改进日志格式和日志级别控制
- 增加更完整的错误处理和连接状态检查
- 整理 install/export CMake 配置

## License

当前仓库暂未添加 LICENSE 文件。如需用于生产或分发，建议先补充明确的开源许可证。

## Star History

如果这个项目帮你理解了 muduo、Reactor 或 Linux 网络编程，欢迎点一个 star。你的反馈会让这个项目继续变得更好。
