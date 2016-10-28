#include "stdafx.h"
#include <iostream>
#include <memory>
#include <string>
#include <native/native.h>
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"iphlpapi.lib")
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Userenv.lib")

using namespace native;

#include <fcntl.h>

int main() {
    auto client = net::tcp::create();
    client->connect("127.0.0.1", 8080, [=](error e){
        client->write("GET / HTTP/1.1\r\n\r\n", [=](error e){
            std::shared_ptr<std::string> response(new std::string);
            client->read_start([=](const char* buf, ssize_t len){
                if(len < 0) // EOF
                {
                    std::cout << *response << std::endl;
                    client->close([](){});
                }
                else
                {
                    response->append(std::string(buf, len));
                }
            });
        });
    });

    return run();
}
