#include "stdafx.h"
#include <iostream>
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"iphlpapi.lib")
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Userenv.lib")
#include <native/native.h>
using namespace native::http;

int main() {
    http server;
    int port = 8080;
    int r = server.listen("0.0.0.0", port, [](request& req, response& res) {
        std::string body = req.get_body(); // Now you can write a custom handler for the body content.
        res.set_status(200);
        res.set_header("Content-Type", "text/plain");
        res.end("C++ FTW\n");
	});
	if (r)
		return r; // Failed to run server.

    std::cout << "Server running at http://0.0.0.0:" << port << "/" << std::endl;
    return native::run();
}
