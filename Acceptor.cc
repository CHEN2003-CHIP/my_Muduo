#include "Acceptor.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "Logger.h"
#include <unistd.h>
static int createNonblocking()
{
    int sockfd=::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
    if(sockfd<0)
    {
        LOG_FATAL("ACCEPTOR::CREATENONBLOCKING::%d EORROR\n",__LINE__);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport):
    loop_(loop),
    acceptSocket_(createNonblocking()),//create socket
    acceptChannel_(loop_,acceptSocket_.fd()),
    listenning_(false)
    //idleFd_()

{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);//bind socket
    
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
     
}
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();//register read event;


}


void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd=acceptSocket_.accept(&peerAddr);
    if(connfd>=0)
    {
        if(newConnectionCallback_)
        {
            newConnectionCallback_(connfd,peerAddr);
        }
        else{
            ::close(connfd);
        }
    }
    else{
        LOG_ERROR("ACCEPTOR::HANDLEREAD::%d EORROR\n",__LINE__);

    }
}
