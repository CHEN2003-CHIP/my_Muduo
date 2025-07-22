#include "EPollPoller.h"
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include "Channel.h"
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

const int kNew=-1;//还没加入epoll
const int kAdded=1;//加入epoll
const int kDeleted=2;

EPollPoller::EPollPoller(EventLoop* loop):
    Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if(epollfd_<0){
        LOG_FATAL("EPOLLPOLLER::epoll create error");

    }
}

EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

void EPollPoller::updateChannel(Channel* channel){
   
    int index=channel->index();
    if(index==kNew || index==kDeleted){
        if(index==kNew){
            int fd=channel->fd();
            channels_[fd]=channel;
        }
        else{}
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);

    }
    else{
        int fd=channel->fd();
        if(channel->isNoneEvent()){
            
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);

        }
        else{
            update(EPOLL_CTL_MOD,channel);
        }
    }
}
void EPollPoller::removeChannel(Channel* channel){
    int fd=channel->fd();
    channels_.erase(fd);
    if(channel->index()==kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}
void EPollPoller::update(int operation,Channel* channel){
    epoll_event event;
    memset(&event,0,sizeof event);
    event.events=channel->events();
    event.data.ptr=channel;
   
    int fd=channel->fd();
    if(::epoll_ctl(epollfd_,operation,fd,&event)<0){
        if(operation==EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error\n");

        }
        else{
            LOG_FATAL("epoll_ctl add or mod error\n");
        }
    }
}

Timestamp EPollPoller::poll(int timeoutMs,ChannelList* activeChannels){
    LOG_INFO("Epollpoller::poll is doing\n");
    int numEvents=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    Timestamp now(Timestamp::now());
    int savedErrno = errno;

    if(numEvents>0){
        fillActiveChannels(numEvents,activeChannels);
        if(static_cast<int>(numEvents)==events_.size()){
            events_.resize(2*events_.size());
        }
    }
    else if(numEvents==0){
        LOG_ERROR("EPOLLPOLLER::POLL IS TIMEOUT\n");
    }//超时{}
    else{
        // 错误处理（EINTR为正常中断）
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_ERROR("EpollPoller::poll()");
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels) const{
    for(int i=0;i<numEvents;i++){
        Channel* channel=static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}
