#ifndef __NET_H__
#define __NET_H__

#include "base.h"
#include "callback.h"

namespace native
{
    namespace net
    {
        typedef sockaddr_in ip4_addr;
        typedef sockaddr_in6 ip6_addr;

        inline error from_ip4_addr(ip4_addr* src, std::string& ip, int& port)
        {
            char dest[16];
			error err = uv_ip4_name(src, dest, 16);
			if(!err)
            {
                ip = dest;
                port = static_cast<int>(ntohs(src->sin_port));
            }
            return err;
        }

        inline int from_ip6_addr(ip6_addr* src, std::string& ip, int& port)
        {
            char dest[46];
			error err = uv_ip6_name(src, dest, 46);
            if (!err)
			{
                ip = dest;
                port = static_cast<int>(ntohs(src->sin6_port));
            }
            return err;
        }
    }
}

#endif
