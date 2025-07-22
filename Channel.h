#pragma once
#include "Timestamp.h"
#include "noncopyable.h"
#include <functional>
#include <memory>
#include  "EventLoop.h"
//class EventLoop;

class Channel:noncopyable{
public:
    using EventCallback=std::function<void()>;
    using ReadEventCallback=std::function<void(Timestamp)>;

    Channel(EventLoop* loop,int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    void setReadCallback(ReadEventCallback cb){
        this->readCallback_=std::move(cb);
    }
    void setWriteCallback(EventCallback cb){
        this->writeCallback_=std::move(cb);
    }
    void setCloseCallback(EventCallback cb){
        this->closeCallback_=std::move(cb);
    }
    void setErrorCallback(EventCallback cb){
        this->errorCallback_=std::move(cb);
    }

    void tie(const std::shared_ptr<void>&);

    int fd() const{return fd_;}
    int events() const {return events_;}
    void set_revents(int revt){revents_=revt;}
    bool isNoneEvent() const{return events_==kNoneEvent;}

    void enableReading(){events_|=kReadEvent;update();}
    void disableReading(){events_ &=~kReadEvent;update();}
    void enableWriting(){events_ |=kWriteEvent;update();}
    void disableWriting(){events_ &= ~kWriteEvent;update();}
    void disableAll(){events_ =kNoneEvent;update();}

    bool isWriting() const{return events_ & kWriteEvent;}
    bool isReading() const{return events_ & kReadEvent;}

    int index(){return index_;}
    void set_index(int idx){index_=idx;}

    EventLoop* ownerLoop(){return loop_;}
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;
    int events_;//感兴趣的事情
    int revents_;//正在发生的事情
    int index_;

    /*
    tie_ 是 muduo 中解决事件处理期间对象生命周期管理的精巧设计：
    通过 weak_ptr 保存上层对象的弱引用
    事件处理前尝试提升为临时 shared_ptr
    提升成功则保证对象在处理期间存活
    提升失败则跳过处理（对象已销毁）
    完美平衡了线程安全和资源效率
    */
    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;


};