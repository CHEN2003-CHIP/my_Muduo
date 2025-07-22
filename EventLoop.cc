#include "EventLoop.h"
#include "Logger.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include "Channel.h"

__thread EventLoop* t_loopInThisThread=nullptr;
const int kPollTimeMs=10000;

int createEventfd(){
    int evtfd=::eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);
    if(evtfd<0){
        LOG_FATAL("EventLoop::createFd error\n");

    }
    return evtfd;
}

EventLoop::EventLoop():
    looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this,wakeupFd_)),
    currentActiveChannel_(nullptr)
{
    LOG_INFO("EventLoop::create %p in thread %d\n",this,threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("already have EventLoop\n");
    }
    else{
        t_loopInThisThread=this;
    }
    //enable read
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

void EventLoop::handleRead()
{
    uint64_t one=1;
    ssize_t n=read(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one){
        LOG_ERROR("EVENTLOOP::READHANDLE ERROR\n");
    }
}

void EventLoop::loop(){
    looping_=true;//start observer
    quit_=false;//not quit
    LOG_INFO("EVENTLOOP :: LOOP IS START\n");

    while(!quit_){
        activeChannels_.clear();
        pollerReturnTime_=poller_->poll(kPollTimeMs,&activeChannels_);
        for(Channel* channel:activeChannels_){
            channel->handleEvent(pollerReturnTime_);
        }

        doPendingFunctors();//execute some cb
    }
}

void EventLoop::quit(){
    quit_=true;
    if(!isInLoopThread()){
        wakeup();
    }
}

// execute the cb in this loop
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()){
        cb();
    }
    else{
        queueInLoop(cb);
    }
}
// put the cb into the queue,wake up the thread who has the loop and execute the cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}
// wake up the thread
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("EVENTLOOP::WAKEUP ERROR\n");
    }
}
// use the poller
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_=true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor& functor : functors){
        functor();
    }
    callingPendingFunctors_=false;
}