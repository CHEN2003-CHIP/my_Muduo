#include "Socket.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Logger.h"
#include <netinet/tcp.h>
#include <strings.h>
#include "InetAddress.h"
Socket::~Socket(){
    close(sockfd_);
};

void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0!=::bind(sockfd_,(sockaddr*)(localaddr.getSockAddr()),sizeof(sockaddr_in)))
    {
        LOG_FATAL("SOCKET::BIND ERROR\n");
    }
}
void Socket::listen()
{
    if(0!=::listen(sockfd_,1024))
    {
        LOG_FATAL("SOCKET:: LISTEN ERROR\n");
    }
}
int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len=sizeof addr;
    bzero(&addr,sizeof addr);
    int confd=::accept4(sockfd_,(sockaddr*)&addr,&len,SOCK_CLOEXEC|SOCK_NONBLOCK);
    if(confd>=0)
    {
        peeraddr->setSockAddr(addr);
    }
    return confd;
}
void Socket::shutdownWrite()
{
    if(shutdown(sockfd_,SHUT_WR)<0)
    {
        LOG_ERROR("SOCKET::SHUTDOWNWR ERROR\n");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);

}
void Socket::setReuseAddr(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof optval);
}
void Socket::setReusePort(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof optval);
}
void Socket::setKeepAlive(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof optval);
}

int Socket::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen=sizeof optval;
    if(::getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&optval,&optlen)<0)
    {
        return errno;
    }
    else{
        return optval;
    }
}