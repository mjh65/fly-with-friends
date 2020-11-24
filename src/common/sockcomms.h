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

#pragma once

#ifdef TARGET_WINDOWS
#include <WinSock2.h>
#else
//#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "iengine.h"
#include "fwfsocket.h"
#include "datagrams.h"
#include <future>
#include <list>

namespace fwf {

// Interface implemented by the UDP socket owner to handle incoming datagrams

class UdpSocketOwner
{
public:
    virtual void IncomingDatagram(AddressedDatagram dgin) = 0;
};

// Local UDP socket which listens for incoming datagrams, and sends
// datagrams when requested.

class UdpSocketLocal
{
public:
    UdpSocketLocal(UdpSocketOwner * owner, const std::string & id, int port = 0);
    ~UdpSocketLocal();

    void QueueDatagram(std::shared_ptr<AddressedDatagram> & dg, bool sendNow = true);
    void SendAll();
    void Shutdown();
    unsigned int RcvdDatagramCount() { std::lock_guard<std::mutex> lock(guard); return recvPacketCount; }

protected:
    void OpenAndBindSocket();
    void CloseSocket();
    bool AsyncReceiver();
    int WaitReceive();
    bool AsyncSender();
    int SendQueued();

private:
    UdpSocketOwner                  * const owner;
    std::string                     const name;
    bool                            running;
    int                             port;
    unsigned int                    recvPacketCount;
    unsigned int                    sendPacketCount;
    std::future<bool>               asyncRecvComplete;
    std::future<bool>               asyncSendComplete;
#ifdef TARGET_WINDOWS
    SOCKET                          udpSocket;
#else
    int                             udpSocket;
#endif
    sockaddr_in                     socketAddress;
    std::list<std::shared_ptr<AddressedDatagram> >
                                    sendQueue;
    std::mutex                      guard;
    std::mutex                      cvGuard;
    std::condition_variable         senderBlock;
#ifndef NDEBUG
    char                            pktDebugBuffer[2 * MAX_DATAGRAM_LEN + 1];
#endif
};

}

