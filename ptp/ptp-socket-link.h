/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Tinghui Wang <tinghui.wang@wsu.edu>
 * 
 * Header file for maintanence structure for socket links
 */

#ifndef PTP_SOCKET_LINK_H
#define PTP_SOCKET_LINK_H

#include "ns3/core-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

class SocketLink {
public:
  SocketLink(
    const uint16_t hostId,
    const uint16_t dstId,
    const Ipv4Address hostIp,
    const uint16_t hostPort,
    const Ipv4Address dstIp,
    const uint16_t dstPort,
    const Ptr<Socket> sock
  );
  uint16_t getHostId();
  uint16_t getDstId();
  uint16_t getHostPort();
  uint16_t getDstPort();
  Ipv4Address getHostIp();
  Ipv4Address getDstIp();
  Ptr<Socket> getSocket();

private:
  const uint16_t m_hostId;    //< Host node ID
  const uint16_t m_dstId;     //< Destination node ID
  const Ipv4Address m_hostIp; //< Host IPv4 Address
  const uint16_t m_hostPort;  //< Host UDP Port
  const Ipv4Address m_dstIp;  //< Destination IPv4 Address
  const uint16_t m_dstPort;   //< Destination UDP Port
  const Ptr<Socket> m_sock;   //< Corresponding Socket Pointer
};

#endif /* PTP_SOCKET_LINK_H */
