//
// Created by Дима Щипицын on 06/11/2021.
//

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "ProxyServer.h"

#define MAX_LENGTH_QUERY 1024
#define HEADER_LENGTH 5
#define COM_QUERY 3

struct event_base       *ProxyServer::eventBase_;
struct evconnlistener   *ProxyServer::connListener_;
struct sockaddr_storage ProxyServer::listenOnAddr_;
struct sockaddr_storage ProxyServer::connectToAddr_;
int                     ProxyServer::connectToAddrLen_;

ProxyServer::ProxyServer(const std::string& proxyAddr, const std::string& serverAddr) {
    // парсим адрес формата "127.0.0.1:80" в структуру sockaddr
    int listenOnAddrLen = sizeof(struct sockaddr_storage);
    memset(&listenOnAddr_, 0, listenOnAddrLen);
    if (evutil_parse_sockaddr_port(proxyAddr.c_str(),
                                   (struct sockaddr*)&listenOnAddr_, &listenOnAddrLen) < 0 ) {
        throw std::runtime_error("Parse listen_addr fail");
    }

    connectToAddrLen_ = sizeof(struct sockaddr_storage);
    memset(&connectToAddr_, 0, connectToAddrLen_);
    if (evutil_parse_sockaddr_port(serverAddr.c_str(),
                                   (struct sockaddr*)&connectToAddr_, &connectToAddrLen_) < 0 ) {
        throw std::runtime_error("Parse connect_addr fail");
    }

    eventBase_ = event_base_new();
    if ( !eventBase_ ) {
        perror("event_base_new()");
        throw std::runtime_error("Event base create fail");
    }

    // связываем слушателя с адресом и назначаем ему коллбэк для события нового подключения
    connListener_ = evconnlistener_new_bind(eventBase_, acceptCb, nullptr,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE,
                                            -1, (struct sockaddr*)&listenOnAddr_, listenOnAddrLen);

    if ( !connListener_ ) {
        throw std::runtime_error("Listener create fail");
    }

    // запукаем цикл обработки событий
    event_base_dispatch(eventBase_);
}

ProxyServer::~ProxyServer() {
    // завершаем цикл и чистим за собой
    event_base_loopexit(eventBase_, nullptr);
    evconnlistener_free(connListener_);
    event_base_free(eventBase_);
}


// header
// 3 байта длина запроса
// 1 байт номер пакета
// 1 байт идентификатор команды
// COM_QUERY команда запроса
bool ProxyServer::is_sql_query(const uint8_t *header) {
    uint8_t command = header[4];
    if ( command == COM_QUERY) {
        return true;
    }
    return false;
}

int ProxyServer::query_length(const uint8_t *header) {
    // 3 байта payload_length
    int len = ((int)header[0] | (int)header[1] << 8 | (int)header[2] << 16);
    return len;
}


// печатаем в консоль строку запроса
void ProxyServer::query_log(unsigned char *query) {
    std::cout << query << std::endl;
}

void ProxyServer::readCb(struct bufferevent *bev, void *ctx) {
    struct  bufferevent *partner;
    struct  evbuffer *src, *dst;
    size_t  len;
    uint8_t header[HEADER_LENGTH];
    unsigned char buffer[MAX_LENGTH_QUERY + 1] = {0};

    src = bufferevent_get_input(bev);
    len = evbuffer_get_length(src);

    partner = reinterpret_cast<struct bufferevent *>(ctx);
    if (!partner) {
        // чистим буфер
        evbuffer_drain(src, len);
        return;
    }

    // копируем хедер
    evbuffer_copyout(src, header, HEADER_LENGTH);
    if ( is_sql_query(header) ) {
        int n = query_length(header) + 5;
        evbuffer_copyout(src, buffer, n < MAX_LENGTH_QUERY ? n : MAX_LENGTH_QUERY);
        query_log(&buffer[5]);
    }

    // копируем буффер входящего запроса в исходящий
    dst = bufferevent_get_output(partner);
    evbuffer_add_buffer(dst, src);
}

void ProxyServer::eventCb(struct bufferevent *bev, short what, void *ctx) {
    auto *partner = reinterpret_cast<struct bufferevent *>(ctx);

    // если соединение разорвано
    if ( what & (BEV_EVENT_EOF | BEV_EVENT_ERROR) ) {
        if (partner) {
            // дочитываем все что осталось
            readCb(bev, ctx);

            if ( evbuffer_get_length(bufferevent_get_output(partner)) > 0 ) {
                bufferevent_disable(partner, EV_READ);
            } else {
                bufferevent_free(partner);
            }
        }
        bufferevent_free(bev);
    }
}

void ProxyServer::acceptCb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *a, int slen, void *p) {
    // не использую это, поэтому привел к void чтобы компилятор ворнингами не сыпал
    (void)listener; (void)a; (void)slen; (void)p;

    struct bufferevent *b_out, *b_in;

    // принимаем новое соединение от клиента
    b_in = bufferevent_socket_new(eventBase_, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

    // создаем новый сокет для подключения к серверу
    b_out = bufferevent_socket_new(eventBase_, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);

    assert(b_in && b_out);

    // соединяем b_out с адресом сервера
    if (bufferevent_socket_connect(b_out, (struct sockaddr*)&connectToAddr_, connectToAddrLen_) < 0) {
        perror("bufferevent_socket_connect");
        bufferevent_free(b_out);
        bufferevent_free(b_in);
        return;
    }

    // назначаем колбэки на каждый сокет
    // коллбэк одинаковый, но аргументы при вызове b_in, b_out
    // будут меняться местами при вызове на каждом из сокетов
    bufferevent_setcb(b_in, readCb, nullptr, eventCb, b_out);
    bufferevent_setcb(b_out, readCb, nullptr, eventCb, b_in);

    // включаем евенты при которых будет совершаться коллбэк
    bufferevent_enable(b_in, EV_READ|EV_WRITE);
    bufferevent_enable(b_out, EV_READ|EV_WRITE);
}