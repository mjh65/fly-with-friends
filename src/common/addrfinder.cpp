/* Simple C library for querying your IP address using http://ipify.org
 *
 * Copyright (c) 2015-2016  Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef TARGET_WINDOWS
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <io.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include "addrfinder.h"
#include "fwfporting.h"

namespace fwf {

static const char* const IPIFY_HOST = "api.ipify.org";

IPAddrFinder::IPAddrFinder()
{
    publicAddress = std::async(std::launch::async, &IPAddrFinder::AsyncGetPublicAddress, this);
}

IPAddrFinder::~IPAddrFinder()
{
    while ((publicAddress.valid()) && (publicAddress.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
}

std::string IPAddrFinder::Finished()
{
    std::string addr;
    if (publicAddress.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        addr = publicAddress.get();
    }
    return addr;
}

std::string IPAddrFinder::AsyncGetPublicAddress()
{
    std::string addr;
    char addrStr[256];
    if (!ipify(addrStr, sizeof(addrStr)))
    {
        int a0,a1,a2,a3;
        if (SSCANF(addrStr, "%d.%d.%d.%d", &a0, &a1, &a2, &a3) == 4)
        {
            addr = addrStr;
        }
    }
    return addr;
}

int IPAddrFinder::ipify(char *addr, size_t len)
{
    SocketNumber sd;
    int ret;

    ret = ipify_connect(sd);
    if (!ret)
    {
        ret = ipify_query(sd, addr, len);
        ret |= ipify_disconnect(sd);
    }
    return ret;
}

/*
 * Connect to api.ipify.org using either address protocol supported.  We
 * want to connect using TCP, so ask getaddrinfo() for a TCP connection
 * over either IPv4 or IPv6, then use the first successful connection.
 */
int IPAddrFinder::ipify_connect(SocketNumber& sd)
{
    struct addrinfo *result, *rp;
    struct addrinfo hints;
    int rc;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(IPIFY_HOST, "http", &hints, &result);
    if (rc || !result) {
        return -1;
    }

    for (rp = result; rp; rp = rp->ai_next) {
        sd = socket(rp->ai_family, rp->ai_socktype, 0);
        if (sd < 0) {
            continue;
        }

        rc = connect(sd, result->ai_addr, (int)result->ai_addrlen);
        if (rc) {
#ifdef TARGET_WINDOWS
            closesocket(sd);
#else
            close(sd);
#endif
            sd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    return (sd > 0) ? 0 : -1;
}

int IPAddrFinder::ipify_query(SocketNumber sd, char *addr, size_t len)
{
    std::string req;
    req = std::string("GET / HTTP/1.0\r\nHost: ") + IPIFY_HOST + "\r\nUser-Agent: FWF/1.0\r\n\r\n";

    int rc;
    char tmp[sizeof(struct in6_addr)];
    char buf[512], *ptr;
    int domain;

    rc = send(sd, req.c_str(), (int)req.size(), 0);
    if (rc < 0) {
        return -1;
    }

    rc = recv(sd, buf, sizeof(buf), 0);
    if (rc < 0) {
        return -1;
    }
    buf[rc] = 0;

    ptr = strstr(buf, "200 OK");
    if (!ptr) {
        return 1;
    }

    ptr = strstr(ptr, "\r\n\r\n");
    if (!ptr) {
        return 1;
    }
    ptr += 4;

    domain = AF_INET;
    if (!inet_pton(domain, ptr, tmp)) {
        domain = AF_INET6;
        if (!inet_pton(domain, ptr, tmp)) {
            return 1;
        }
    }

    if (!inet_ntop(domain, tmp, addr, len)) {
        return 1;
    }

    return 0;
}

int IPAddrFinder::ipify_disconnect(SocketNumber sd)
{
#ifdef TARGET_WINDOWS
    shutdown(sd, SD_BOTH);
    return closesocket(sd);
#else
    shutdown(sd, SHUT_RDWR);
    return close(sd);
#endif
}

}
