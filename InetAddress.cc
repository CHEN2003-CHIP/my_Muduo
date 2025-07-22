#include "InetAddress.h"

#include <strings.h>
#include <iostream>
#include <string.h>
using namespace std;

InetAddress::InetAddress(uint16_t port, string ip){
    bzero(&addr_,sizeof(addr_));
    addr_.sin_family=AF_INET;
    addr_.sin_port=htons(port);
    addr_.sin_addr.s_addr=inet_addr(ip.c_str());

}


string InetAddress::toIp() const{
    char buf[64]={0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    return buf;
}

string InetAddress::toIpPort() const{
    char buf[64]={0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    int end=strlen(buf);
    uint16_t port=ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);
    return buf;
}

uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}

/*
int  main(){
    InetAddress addr(8080);
    cout<<addr.toIpPort()<<endl;
    return  0;
}
*/