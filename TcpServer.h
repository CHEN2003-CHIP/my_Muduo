#pragma once 

#include "EventLoop.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "noncopyable.h"
#include <functional>
#include <string>
#include <memory>
#include "Callbacks.h"
#include <atomic>
#include <unordered_map>
#include "TcpConnection.h"

class TcpServer:noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;
    enum Option {
        kNoReusePort,
        kReusePort,
    };
    TcpServer(EventLoop* loop,const InetAddress& listenAddr,
                const std::string& nameArg,
                Option option=kNoReusePort);
    ~TcpServer();
    void setThreadNum(int numThread);
    void setThreadInitCallback(const ThreadInitCallback& cb){threadInitCallback_=cb;}

    void start();
    void setConnectionCallback(const ConnectionCallback& cb)
    {connectionCallback_=cb;}

    void setMessageCallback(const MessageCallback& cb)
    {messageCallback_=cb;}

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {writeCompleteCallback_=cb;}

private:
    void newConnection(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    using ConnectionMap=std::unordered_map<std::string,TcpConnectionPtr>;
    EventLoop* loop_;
    const std::string  ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;

    std::atomic_int shared_;
    int nextConnId_;
    ConnectionMap connections_;
};