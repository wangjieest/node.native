#ifndef __TCP_H__
#define __TCP_H__

#include "base.h"
#include "handle.h"
#include "stream.h"
#include "net.h"
#include "callback.h"

namespace native
{
    namespace net
    {
        class tcp : public native::base::stream
        {
        public:
            template<typename X>
            tcp(X* x)
                : stream(x)
            { }

        public:
            tcp()
                : native::base::stream(new uv_tcp_t)
            {
                uv_tcp_init(uv_default_loop(), get<uv_tcp_t>());
            }

            tcp(native::loop& l)
                : native::base::stream(new uv_tcp_t)
            {
                uv_tcp_init(l.get(), get<uv_tcp_t>());
            }

            static std::shared_ptr<tcp> create()
            {
                return std::shared_ptr<tcp>(new tcp);
            }

            // TODO: bind and listen
            static std::shared_ptr<tcp> create_server(const std::string& ip, int port) {
                return nullptr;
            }

			error nodelay(bool enable) { return uv_tcp_nodelay(get<uv_tcp_t>(), enable?1:0); }
			error keepalive(bool enable, unsigned int delay) { return uv_tcp_keepalive(get<uv_tcp_t>(), enable?1:0, delay); }
			error simultanious_accepts(bool enable) { return uv_tcp_simultaneous_accepts(get<uv_tcp_t>(), enable?1:0); }

			error bind(const std::string& ip, int port) {
                sockaddr_in sock; 
                return uv_ip4_addr(ip.c_str(), port, &sock) 
                    || uv_tcp_bind(get<uv_tcp_t>(), (sockaddr*)&sock, 0); 
            }
			error bind6(const std::string& ip, int port) {
                sockaddr_in6 sock; 
                return uv_ip6_addr(ip.c_str(), port, &sock) 
                    || uv_tcp_bind(get<uv_tcp_t>(), (sockaddr*)&sock, UV_TCP_IPV6ONLY); 
            }

			error connect(const std::string& ip, int port, std::function<void(error)>&& callback)
            {
                sockaddr_in sock;
                callbacks::store(get()->data, native::internal::uv_cid_connect, std::move(callback));
                return uv_ip4_addr(ip.c_str(), port, &sock)
                    || uv_tcp_connect(new uv_connect_t, get<uv_tcp_t>(), (sockaddr*)&sock, [](uv_connect_t* req, int status) {
                    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_connect, status);
                    delete req;
                });
            }

			error connect6(const std::string& ip, int port, std::function<void(error)>&& callback)
            {
                sockaddr_in6 sock;
                callbacks::store(get()->data, native::internal::uv_cid_connect6, std::move(callback));
                return uv_ip6_addr(ip.c_str(), port, &sock)
                    || uv_tcp_connect(new uv_connect_t, get<uv_tcp_t>(), (sockaddr*)&sock, [](uv_connect_t* req, int status) {
                    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_connect6, status);
                    delete req;
                });
            }

			error getsockname(bool& ip4, std::string& ip, int& port)
            {
                struct sockaddr_storage addr;
                int len = sizeof(addr);
				error err = uv_tcp_getsockname(get<uv_tcp_t>(), reinterpret_cast<struct sockaddr*>(&addr), &len);
				if(!err)
                {
                    ip4 = (addr.ss_family == AF_INET);
                    if(ip4) return from_ip4_addr(reinterpret_cast<ip4_addr*>(&addr), ip, port);
                    else return from_ip6_addr(reinterpret_cast<ip6_addr*>(&addr), ip, port);
                }
                return err;
            }

			error getpeername(bool& ip4, std::string& ip, int& port)
            {
                struct sockaddr_storage addr;
                int len = sizeof(addr);
				error err = uv_tcp_getpeername(get<uv_tcp_t>(), reinterpret_cast<struct sockaddr*>(&addr), &len);
				if(!err)
                {
                    ip4 = (addr.ss_family == AF_INET);
                    if(ip4) return from_ip4_addr(reinterpret_cast<ip4_addr*>(&addr), ip, port);
                    else return from_ip6_addr(reinterpret_cast<ip6_addr*>(&addr), ip, port);
                }
                return err;
            }
        };
    }
}

#endif
