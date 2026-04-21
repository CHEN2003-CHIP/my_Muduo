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
    LOG_INFO("recv %d bytes: %s\n", static_cast<int>(msg.size()), msg.c_str());
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
