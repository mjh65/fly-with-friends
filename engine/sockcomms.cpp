/*
 *   Fly-With-Friends - X-Plane plugin for simple group flying
 *   Copyright (C) 2020 Michael Hasling <michael.hasling@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "sockcomms.h"
#include "ilogger.h"
#ifndef TARGET_WINDOWS
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace fwf {

static const bool infoLogging = true;
static const bool verboseLogging = false;
static const bool debugLogging = false;

UdpSocketLocal::UdpSocketLocal(UdpSocketOwner * o, const std::string & n, int p)
:   owner(o),
    name(n),
    running(true),
    port(p),
    recvPacketCount(0),
    sendPacketCount(0)
{
    OpenAndBindSocket(); // throws exception on failure
    asyncRecvComplete = std::async(std::launch::async, &UdpSocketLocal::AsyncReceiver, this);
    asyncSendComplete = std::async(std::launch::async, &UdpSocketLocal::AsyncSender, this);
}

UdpSocketLocal::~UdpSocketLocal()
{
    Shutdown();
    while ((asyncSendComplete.valid()) && (asyncSendComplete.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
    while ((asyncRecvComplete.valid()) && (asyncRecvComplete.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready)) {}
}

void UdpSocketLocal::QueueDatagram(std::shared_ptr<AddressedDatagram> & dg, bool sendNow)
{
    FWF_LOG_VERBOSE(verboseLogging,"%s: queue datagram for sending", name.c_str());
    {
        std::lock_guard<std::mutex> lock(guard);
        sendQueue.push_back(dg);
    }
    if (sendNow) SendAll();
}

void UdpSocketLocal::SendAll()
{
    FWF_LOG_VERBOSE(verboseLogging,"%s: signalling async sender thread", name.c_str());
    std::unique_lock<std::mutex> lock(cvGuard);
    senderBlock.notify_one();
}

void UdpSocketLocal::Shutdown()
{
    running = false;
    CloseSocket();
    std::unique_lock<std::mutex> lock(cvGuard);
    senderBlock.notify_one();
}

void UdpSocketLocal::OpenAndBindSocket()
{
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifdef TARGET_WINDOWS
    if (udpSocket == INVALID_SOCKET)
    {
        FWF_LOG_ERROR("%s: UDP socket bind failed", name.c_str());
        throw - 1;
    }
#endif

    memset(&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    socketAddress.sin_port = htons(port);
#ifdef TARGET_WINDOWS
    if (bind(udpSocket, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) == SOCKET_ERROR)
#else
    if (bind(udpSocket, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) == -1)
#endif
    {
        FWF_LOG_ERROR("%s: UDP socket bind failed", name.c_str());
#ifdef TARGET_WINDOWS
        closesocket(udpSocket);
#else
        close(udpSocket);
#endif
        throw - 1;
    }

    struct sockaddr_in s;
#ifdef TARGET_WINDOWS
    int ls = sizeof(s);
#else
    socklen_t ls = sizeof(s);
#endif
    if (getsockname(udpSocket, (struct sockaddr*)&s, &ls) == -1)
    {
        FWF_LOG_WARN("%s: UDP socket getsockname failed", name.c_str());
#ifdef TARGET_WINDOWS
        closesocket(udpSocket);
#else
        close(udpSocket);
#endif
        throw - 1;
    }
    else
    {
        port = ntohs(s.sin_port);
        FWF_LOG_INFO(infoLogging, "%s: assigned local port number %u", name.c_str(), port);
    }
}

void UdpSocketLocal::CloseSocket()
{
    std::lock_guard<std::mutex> lock(guard);
#ifdef TARGET_WINDOWS
    if (udpSocket != INVALID_SOCKET)
    {
        closesocket(udpSocket);
        udpSocket = INVALID_SOCKET;
    }
#else
    if (udpSocket != -1)
    {
        close(udpSocket);
        udpSocket = -1;
    }
#endif
}

bool UdpSocketLocal::AsyncReceiver()
{
    while (running)
    {
        int n = WaitReceive();
        if (n < 0) break;
        std::lock_guard<std::mutex> lock(guard);
        recvPacketCount += n;
    }
    CloseSocket();
    return true;
}

int UdpSocketLocal::WaitReceive()
{
    AddressedDatagram dgf(std::make_shared<Datagram>());
    sockaddr_in from;
#ifdef TARGET_WINDOWS
    int fsz = sizeof(from);
#else
    socklen_t fsz = sizeof(from);
#endif
    auto recsize = recvfrom(udpSocket, dgf.Data()->Buffer(), MAX_DATAGRAM_LEN, 0, (struct sockaddr *)&from, &fsz);
    if (recsize < 0) return -1;
    IPv4Address a(ntohl(from.sin_addr.s_addr));
    FWF_LOG_VERBOSE(verboseLogging,"%s: received datagram from %s : %d len=%d", name.c_str(), a.GetQuad().c_str(), ntohs(from.sin_port), recsize);
#ifndef NDEBUG
    static const char * xdig = "0123456789abcdef";
    const unsigned char * p = (unsigned char *)dgf.Data()->Buffer();
    for (size_t i=0, j=0; i<(unsigned)recsize; ++i, j+=2, ++p)
    {
        *(pktDebugBuffer+j) = xdig[(*p)/16];
        *(pktDebugBuffer+j+1) = xdig[(*p)%16];
    }
    *(pktDebugBuffer+(2*recsize)) = 0;
    FWF_LOG_DEBUG(debugLogging,"%s: %s", name.c_str(), pktDebugBuffer);
#endif
    if (recsize < 8) return 0; // must be at least 8 bytes long for seq-number, command, payload-length
    dgf.Data()->SetLength(recsize);
    dgf.SetAddress(SocketAddress(ntohl(from.sin_addr.s_addr), ntohs(from.sin_port)));
    owner->IncomingDatagram(dgf);
    return 1;
}


bool UdpSocketLocal::AsyncSender()
{
    while (1)
    {
        // Wait until there is something to be sent
        {
            std::unique_lock<std::mutex> lock(cvGuard);
            senderBlock.wait_for(lock, std::chrono::milliseconds(1000));
        }
        // If the receiver has stopped (finished) then so should we
        if (!running || (asyncRecvComplete.wait_for(std::chrono::seconds(0)) == std::future_status::ready)) break;
        int n = SendQueued();
        if (n < 0) break;
        std::lock_guard<std::mutex> lock(guard);
        sendPacketCount += n;
    }
    return true;
}

int UdpSocketLocal::SendQueued()
{
    int sent = 0;
    std::shared_ptr<AddressedDatagram> next;
    while (1)
    {
        {
            std::lock_guard<std::mutex> lock(guard);
            if (sendQueue.empty()) break;
            next = sendQueue.front();
            sendQueue.pop_front();
        }
        FWF_LOG_VERBOSE(verboseLogging,"%s: sending datagram to %s : %d", name.c_str(), next->Address().GetQuad().c_str(), next->Address().GetPort());
        sockaddr_in target;
        memset(&target, 0, sizeof(target));
        target.sin_family = AF_INET;
        target.sin_addr.s_addr = htonl(next->Address().GetAsUInt32());
        target.sin_port = htons(next->Address().GetPort());
        if (sendto(udpSocket, next->Data()->Buffer(), next->Data()->Length(), 0, (struct sockaddr *)&target, sizeof(target)) < 0)
        {
            return -1;
        }
        ++sent;
    }
    return sent;
}


}

