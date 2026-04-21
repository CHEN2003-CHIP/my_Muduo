#include "TcpConnection.h"
#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include <errno.h>
#include <memory>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
using namespace std;
using namespace placeholders;
static EventLoop* CHECK_NOTNULL(EventLoop* loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("TCPConnection::CHECK_NOTNULL::loop is NULL\n");
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,const std::string &name,
        int sockfd,const InetAddress& localAddr,
        const InetAddress& peerAddr):
        loop_(CHECK_NOTNULL(loop)),
        name_(name),
        state_(kConnecting),
        reading_(true),
        socket_(new Socket(sockfd)),
        channel_(new Channel(loop,sockfd)),
        localAddr_(localAddr),
        peerAddr_(peerAddr),
        highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError,this));
    socket_->setKeepAlive(true);
}
TcpConnection:: ~TcpConnection()
{

}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno=0;
    ssize_t n=inputBuffer_.readFd(channel_->fd(),&savedErrno);
    if(n>0)
    {
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);

    }
    else if(n==0)
    {
        handleClose();
    }
    else{
        errno=savedErrno;
        LOG_ERROR("TCPCONNECTION::HANDLEREAD ERROR\n");
        handleError();
    }
}  
void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int savedErrno=0;
        ssize_t n=outputBuffer_.writeFd(channel_->fd(),&savedErrno);
        if(n>0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes()==0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if(state_==kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TCPCONNECTION::HANDLEWRITE ERROR\n");
        }
    }
    else{
        LOG_ERROR("TCPCONNECTION::HANDLEWRITE NO MORE WRITING ERROR\n");
    }
}
void TcpConnection::handleClose()
{
    LOG_INFO("TCPCONNECTION::HANDLECLOSE FD=%d state_=%d\n",channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);
    closeCallback_(connPtr);
}
void TcpConnection::handleError()
{
    int err=0;
    int optval;
    socklen_t optlen=sizeof optval;
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)<0)
    {
        err= errno;
    }
    else{
        err= optval;
    }
    
    LOG_ERROR("TCPCONNECTION::HANDLEERROR ERR=%d\n",err);
}

void TcpConnection::send(const std::string & buf)
{
    if(state_==kConnected)//only the connection is connected 
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf);

        }
        else{
            loop_->runInLoop(std::bind(static_cast<void (TcpConnection::*)(const std::string&)>(&TcpConnection::sendInLoop),
                                       shared_from_this(),
                                       buf));

        }
    }
}

void TcpConnection::send(const void* message,int len)
{
    if(message == nullptr || len <= 0)
    {
        return;
    }
    send(std::string(static_cast<const char*>(message),len));
}

void TcpConnection::sendInLoop(const std::string& message)
{
    sendInLoop(message.data(),message.size());
}

void TcpConnection::sendInLoop(const void* data,size_t len)
{
    ssize_t nwrote=0;
    ssize_t remaining=len;
    bool faultError=false;
    if(state_==kDisconnected)
    {
        LOG_ERROR("DISCONNECTED WRITTING ERROR\n");
        return;
    }
    if(!channel_->isWriting() && outputBuffer_.readableBytes()==0)
    {
        nwrote=::write(channel_->fd(),data,len);
        if(nwrote>=0)
        {
            remaining=len-nwrote;
            //send all /all work is done
            if(remaining==0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));

            }
        }
        else
        {
            nwrote=0;
            if(errno!=EWOULDBLOCK)
            {
                LOG_ERROR("TCPCONNECTION::SENDINLOOP ERROR\nfuck!\n");
                if(errno==EPIPE || errno==ECONNRESET)
                {
                    faultError=true;
                }
            }
        }
    }
    if(!faultError && remaining>0)
    {
        size_t oldLen=outputBuffer_.readableBytes();
        if(oldLen+remaining>=highWaterMark_ && 
            oldLen<highWaterMark_ &&
            highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oldLen+remaining));

        }
        outputBuffer_.append((char*)(data)+nwrote,remaining);
        if(!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}
void TcpConnection::connectDestroyed()
{
    if(state_==kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());

    }
    channel_->remove();
}

void TcpConnection::shutdown()
{
    if(state_==kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}
void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}
