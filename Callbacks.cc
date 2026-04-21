#include "Callbacks.h"
#include "Buffer.h"
#include "TcpConnection.h"
#include "Timestamp.h"

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    (void)conn;
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime)
{
    (void)conn;
    (void)receiveTime;
    buffer->retrieveAll();
}
