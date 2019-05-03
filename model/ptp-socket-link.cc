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
 * Implementation of PTP socket link maintanence structure
 */

#include "ns3/core-module.h"
#include "ptp-socket-link.h"

using namespace ns3;

/**
 * @brief Construct a new Socket Link:: Socket Link object
 * 
 * @param txNodeId global sender ID
 * @param rxNodeId global receiver ID
 * @param txIp sender IP Address
 * @param txPort sender binded UDP port
 * @param rxIp receiver IP Address
 * @param rxPort receiver binded UDP port
 * @param sock pointer to the ns3::Socket structure
 */
SocketLink::SocketLink(
  const uint16_t hostId,
  const uint16_t dstId,
  const Ipv4Address hostIp,
  const uint16_t hostPort,
  const Ipv4Address dstIp,
  const uint16_t dstPort,
  const Ptr<Socket> sock
) : m_hostId(hostId),
    m_dstId(dstId),
    m_hostIp(hostIp),
    m_hostPort(hostPort),
    m_dstIp(dstIp),
    m_dstPort(dstPort),
    m_sock(sock)
    {}

uint16_t SocketLink::getHostId() {
  return m_hostId;
}

uint16_t SocketLink::getDstId() {
  return m_dstId;
}

Ipv4Address SocketLink::getHostIp() {
  return m_hostIp;
}

Ipv4Address SocketLink::getDstIp() {
  return m_dstIp;
}

uint16_t SocketLink::getHostPort() {
  return m_hostPort;
}

uint16_t SocketLink::getDstPort() {
  return m_dstPort;
}

Ptr<Socket> SocketLink::getSocket() {
  return m_sock;
}
