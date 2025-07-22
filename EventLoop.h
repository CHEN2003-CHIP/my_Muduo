#pragma once
#include <functional>
#include "Timestamp.h"
#include "noncopyable.h"
#include "CurrentThread.h"
#include <sys/types.h>
#include <atomic>
#include <memory>
#include <mutex>
#include "Poller.h"
#include <vector>
class Channel;
class EventLoop:noncopyable{
public:
    using Functor=std::function<void()>;
    EventLoop();
    ~EventLoop();

    //start a event loop
    void loop();
    //quit the loop
    void quit();

    Timestamp pollReturnTime()const{return pollerReturnTime_;}
    //execute the cb in this loop
    void runInLoop(Functor cb);
    //put the cb into the queue,wake up the thread who has the loop and execute the cb
    void queueInLoop(Functor cb);
    //wake up the thread
    void wakeup();
    //use the poller
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isInLoopThread()const {return threadId_==  CurrentThread::tid();}



private:
    void handleRead();
    void doPendingFunctors();//execute the callback

    using ChannelList=std::vector<Channel*>;
    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t threadId_;
    std::unique_ptr<Poller> poller_;
    Timestamp pollerReturnTime_;

    int wakeupFd_;//*** */
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;
    std::atomic_bool callingPendingFunctors_;
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;

};