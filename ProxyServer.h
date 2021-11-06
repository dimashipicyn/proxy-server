//
// Created by Дима Щипицын on 06/11/2021.
//

#ifndef TEST_DATA_ARMOR_PROXYSERVER_H
#define TEST_DATA_ARMOR_PROXYSERVER_H


#include <memory>
#include <string>
#include <event2/event.h>

class ProxyServer {
public:

    ProxyServer(const std::string& proxyAddr, const std::string& serverAddr);
    ~ProxyServer();
    ProxyServer(const ProxyServer& proxyServer) = delete;
    ProxyServer &operator=(const ProxyServer& proxyServer) = delete;

private:
    // для работы libevent
    static struct event_base            *eventBase_;
    static struct evconnlistener        *connListener_;
    static struct sockaddr_storage      listenOnAddr_;
    static struct sockaddr_storage      connectToAddr_;
    static int                          connectToAddrLen_;

    // коллбэки для libevent
    static void readCb(struct bufferevent *bev, void *ctx);
    static void eventCb(struct bufferevent *bev, short what, void *ctx);
    static void acceptCb(struct evconnlistener *listener, evutil_socket_t fd,
            struct sockaddr *a, int slen, void *p);
};


#endif //TEST_DATA_ARMOR_PROXYSERVER_H
