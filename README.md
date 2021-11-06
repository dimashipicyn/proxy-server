##Simple proxy server

dependences: libevent

build: make

run: ./proxy <listen> <connect>
Example: ./proxy 127.0.0.1:1111 1.2.3.4:80

Собирал на mac os, на других ОС не имел возможности попробовать собрать