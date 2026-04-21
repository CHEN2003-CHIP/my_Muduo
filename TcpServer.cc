#include "TcpServer.h"
#include "Logger.h"
#include <strings.h>
#include "TcpConnection.h"
using namespace std;
using namespace placeholders;
static EventLoop* CHECK_NOTNULL(EventLoop* loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("TCPSERVER::CHECK_NOTNULL::loop is NULL\n");
    }
    return loop;
}
TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
          const std::string &nameArg,
          Option option):
          loop_(CHECK_NOTNULL(loop)),
          ipPort_(listenAddr.toIpPort()),
          name_(nameArg),
          acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)),
          threadPool_(new EventLoopThreadPool(loop,name_)),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          nextConnId_(1),
          shared_(0)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,_1,_2));
}

TcpServer::~TcpServer()
{
    for(auto& item:connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();

        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
}
void TcpServer::setThreadNum(int numThread)
{
    threadPool_->setThreadNum(numThread);
}



void TcpServer::start()
{
    if(shared_++==0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}


void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop* ioLoop=threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName=name_+buf;

    LOG_INFO("TCPSERVER::NEWCONNECTION NAME=%s\n",connName.c_str());

    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen=sizeof local;
    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen)<0){
        LOG_ERROR("TCPSERVER::GETSOCKNAME ERROR\n");
    }
    InetAddress localAddr(local);

    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                        connName,
                        sockfd,
                        localAddr,
                        peerAddr));
    connections_[connName]=conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,_1));
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));


    
}
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("tcpserver::removeconnectioninLoop\n");
    connections_.erase(conn->name());
    EventLoop* ioLoop=conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn)); 
}
