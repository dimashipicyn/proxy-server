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
};


#endif //TEST_DATA_ARMOR_PROXYSERVER_H
