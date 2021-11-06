//
// Created by Дима Щипицын on 06/11/2021.
//

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>

#include "ProxyServer.h"


// для работы libevent
static struct event_base            *eventBase;
static struct evconnlistener        *listener;
static struct sockaddr_storage      listenOnAddr;
static struct sockaddr_storage      connectToAddr;
static int                          connectToAddrLen;

// коллбэки для libevent
static void writeCb(struct bufferevent *bev, void *ctx);
static void readCb(struct bufferevent *bev, void *ctx);
static void eventCb(struct bufferevent *bev, short what, void *ctx);
static void acceptCb(struct evconnlistener *listener, evutil_socket_t fd,
        struct sockaddr *a, int slen, void *p);


ProxyServer::ProxyServer(const std::string& proxyAddr, const std::string& serverAddr) {
    // парсим адрес формата 127.0.0.1:80 в структуру sockaddr
    int listenOnAddrLen = sizeof(struct sockaddr_storage);
    memset(&listenOnAddr, 0, listenOnAddrLen);
    if (evutil_parse_sockaddr_port(proxyAddr.c_str(),
                                   (struct sockaddr*)&listenOnAddr, &listenOnAddrLen) < 0 ) {
        throw std::runtime_error("Parse listen_addr fail");
    }

    connectToAddrLen = sizeof(struct sockaddr_storage);
    memset(&connectToAddr, 0, connectToAddrLen);
    if (evutil_parse_sockaddr_port(serverAddr.c_str(),
                                   (struct sockaddr*)&connectToAddr, &connectToAddrLen) < 0 ) {
        throw std::runtime_error("Parse connect_addr fail");
    }

    eventBase = event_base_new();
    if ( !eventBase ) {
        perror("event_base_new()");
        throw std::runtime_error("Event base create fail");
    }

    // связываем слушателя с адресом и назначаем ему коллбэк для события нового подключения
    listener = evconnlistener_new_bind(eventBase, acceptCb, NULL,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE,
                                       -1, (struct sockaddr*)&listenOnAddr, listenOnAddrLen);

    if ( !listener ) {
        throw std::runtime_error("Listener create fail");
    }

    // запукаем цикл обработки событий
    event_base_dispatch(eventBase);
}

ProxyServer::~ProxyServer() {
    // завершаем цикл и чистим за собой
    event_base_loopexit(eventBase, nullptr);
    evconnlistener_free(listener);
    event_base_free(eventBase);
}

static void writeCb(struct bufferevent *bev, void *ctx) {

}

static void readCb(struct bufferevent *bev, void *ctx) {
    struct bufferevent *partner;
    struct evbuffer *src, *dst;
    size_t len;

    partner = reinterpret_cast<struct bufferevent *>(ctx);
    src = bufferevent_get_input(bev);
    len = evbuffer_get_length(src);

    char buf[10000] = {0};
    evbuffer_copyout(src, buf, len);
    std::cout << "Len: " << len << std::endl;
    //std::cout << buf;
    int n = (buf[0] | (int)buf[1] << 8 | (int)buf[2] << 16);
    int q = buf[4];
    std::cout << "lenght " << n << " query " << q << std::endl;
    dst = bufferevent_get_output(partner);
    evbuffer_add_buffer(dst, src);
    std::cout << "Call readcb" << std::endl;
}

static void eventCb(struct bufferevent *bev, short what, void *ctx) {
    struct bufferevent *partner = reinterpret_cast<struct bufferevent *>(ctx);
    std::cout << "Eventcb call" << std::endl;
}


static void acceptCb(struct evconnlistener *listener, evutil_socket_t fd,
                                struct sockaddr *a, int slen, void *p) {
    struct bufferevent *b_out, *b_in;

    // принимаем новое соединение от клиента
    b_in = bufferevent_socket_new(eventBase, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

    // создаем новый сокет для подключения к серверу
    b_out = bufferevent_socket_new(eventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

    assert(b_in && b_out);

    // соединяем b_out с адресом сервера
    if (bufferevent_socket_connect(b_out, (struct sockaddr*)&connectToAddr, connectToAddrLen) < 0) {
        perror("bufferevent_socket_connect");
        bufferevent_free(b_out);
        bufferevent_free(b_in);
        return;
    }

    // назначаем колбэки на каждый сокет
    // коллбэк одинаковый, но аргументы при вызове b_in, b_out
    // будут меняться местами при вызове на каждом из сокетов
    bufferevent_setcb(b_in, readCb, NULL, eventCb, b_out);
    bufferevent_setcb(b_out, readCb, NULL, eventCb, b_in);

    // включаем евенты при которых будет совершаться коллбэк
    bufferevent_enable(b_in, EV_READ|EV_WRITE);
    bufferevent_enable(b_out, EV_READ|EV_WRITE);
}
