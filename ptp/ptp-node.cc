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
 * This file declears wireless nodes and wired nodes with IEEE 1588 PTP
 * support.
 */

#include "ns3/core-module.h"
#include "ptp-node.h"

using namespace ns3;

PtpNode::PtpNode(
  const uint16_t id,
  const uint16_t masterId,
  const uint16_t hop,
  const Ipv4Address ipv4Address
) : m_nodeId(id),
    m_masterId(masterId),
    m_hopNum(hop),
    m_nodeIpv4Address(ipv4Address)
{
  // Questions remain about what is hop here.
  m_nodeState = INACTIVE;
  m_isGlobalMaster = false;

  // Initialize time stamps
  m_syncTimeAtMaster = NanoSeconds(0);
  m_dreqTimeAtMaster = NanoSeconds(0);
  m_syncRecvTime = NanoSeconds(0);
  m_dreqSendTime = NanoSeconds(0);
  m_offset = NanoSeconds(0);

  m_clockError = ( rand() % 12 ) * 0.012 / 12 + 0.994;
  m_prevOffsetError = 0;
  m_currOffsetError = 0;
  
  // Initialize the number of packets per message type to zero
  for(int j=0; j < 4; j++) {
    m_sentPacket.push_back(0);
    m_receivedPacket.push_back(0);
    m_overheardPacket.push_back(0);
  }
}

uint16_t PtpNode::getNodeHop() {
  return m_hopNum;
}

uint16_t PtpNode::getNodeId() {
  return m_nodeId;
}

uint16_t PtpNode::getMasterId() {
  return m_masterId;
}

Ipv4Address PtpNode::getIpv4Address() {
  return m_nodeIpv4Address;
}

void PtpNode::setGlobalMaster() {
  m_isGlobalMaster = true;
  m_clockError = 1.;
}

bool PtpNode::isGlobalMaster() {
  return m_isGlobalMaster;
}

NodeState_t PtpNode::getState() {
  return m_nodeState;
}

void PtpNode::setState(NodeState_t s) {
  m_nodeState = s;
}

Time PtpNode::getLocalTime() {
  return m_localTime;
}

void PtpNode::setLocalTime(Time simulatorTime) {
  if(!m_isGlobalMaster) {
    // Simulate node clock jitter
    m_localTime = NanoSeconds(
      (simulatorTime.GetNanoSeconds() - m_simulatorTime.GetNanoSeconds()) * m_clockError
      + m_localTime.GetNanoSeconds()
    );
    m_simulatorTime = simulatorTime;
  } else {
    // global master is synchronized to simulator global time
    m_simulatorTime = simulatorTime;
    m_localTime = simulatorTime;
  }
}

Time PtpNode::getSyncTimeAtMaster() {
  return m_syncTimeAtMaster;
}

void PtpNode::setSyncTimeAtMaster(Time time) {
  m_syncTimeAtMaster = time;
}

Time PtpNode::getDreqTimeAtMaster() {
  return m_dreqTimeAtMaster;
}

void PtpNode::setDreqTimeAtMaster(Time time) {
  m_dreqTimeAtMaster = time;
}

Time PtpNode::getSyncRecvTime() {
  return m_syncRecvTime;
}

void PtpNode::setSyncRecvTime(Time time) {
  m_syncRecvTime = time;
}

Time PtpNode::getDreqSendTime() {
  return m_dreqSendTime;
}

void PtpNode::setDreqSendTime(Time time) {
  m_dreqSendTime = time;
}

Time PtpNode::getSyncSendTimeStamp(uint16_t nodeId) {
  unsigned int i = 0;
  while(i < m_neighbors.size() && m_neighbors[i] != nodeId) {
    i++;
  }
  if(i == m_neighbors.size()) {
    std::cerr << "[PtpNode::getSyncSendTimeStamp] Failed to find node " << 
      nodeId << " in the neighbor list " << " of node " << m_nodeId << 
      "." << std::endl;
    return NanoSeconds(0);
  } else {
    return m_syncSendTimeStamps[i];
  }
}

void PtpNode::setSyncSendTimeStamp(Time time, uint16_t nodeId) {
  unsigned int i = 0;
  while(i < m_neighbors.size() && m_neighbors[i] != nodeId) {
    i++;
  }
  if(i < m_neighbors.size()) {
    m_syncSendTimeStamps[i] = time;
  }
}

Time PtpNode::getDreqRecvTimeStamp(uint16_t nodeId) {
  unsigned int i = 0;
  while(i < m_neighbors.size() && m_neighbors[i] != nodeId) {
    i++;
  }
  if(i == m_neighbors.size()) {
    std::cerr << "[PtpNode::getDreqRecvTimeStamp] Failed to find node " << 
      nodeId << " in the neighbor list " << " of node " << m_nodeId << 
      "." << std::endl;
    return NanoSeconds(0);
  } else {
    return m_dreqRecvTimeStamps[i];
  }
}

void PtpNode::setDreqRecvTimeStamp(Time time, uint16_t nodeId) {
  unsigned int i = 0;
  while(i < m_neighbors.size() && m_neighbors[i] != nodeId) {
    i++;
  }
  if(i < m_neighbors.size()) {
    m_dreqRecvTimeStamps[i] = time;
  }
}

int PtpNode::getNumNeighbors() {
  return m_neighbors.size();
}

void PtpNode::addNeighbor(
  uint16_t nodeId, 
  SocketLink *txSocket
) {
  m_dreqRecvTimeStamps.push_back(NanoSeconds(0));
  m_syncSendTimeStamps.push_back(NanoSeconds(0));
  m_neighbors.push_back(nodeId);
  m_sockets.push_back(txSocket);
}

SocketLink *PtpNode::getTxSocket(int index) {
  return m_sockets[index];
}

SocketLink *PtpNode::getTxSocketByNodeId(uint16_t nodeId) {
  unsigned int i = 0;
  while(i < m_neighbors.size() && m_neighbors[i] != nodeId) {
    i++;
  }
  if(i < m_neighbors.size()) {
    return m_sockets[i];
  } else {
    return NULL;
  }
}

void PtpNode::incrementSentPacketCounter(PtpMessageType_t msgType) {
  m_sentPacket[msgType]++;
}

void PtpNode::increaseReceivedPacketCounter(PtpMessageType_t msgType) {
  m_receivedPacket[msgType]++;
}

void PtpNode::calculateOffset(Time masterTime) {
  int64_t clockOffset;
  if(!m_isGlobalMaster) {
    clockOffset = (
      (m_syncRecvTime.GetNanoSeconds() - m_syncTimeAtMaster.GetNanoSeconds()) +
      (m_dreqSendTime.GetNanoSeconds() - m_dreqTimeAtMaster.GetNanoSeconds())
    ) / 2;
    m_offset = NanoSeconds(clockOffset);
    m_prevOffsetError = std::abs((
      m_localTime.GetNanoSeconds() - masterTime.GetNanoSeconds()
    ));
    m_localTime -= m_offset;
    m_currOffsetError = std::abs((
      m_localTime.GetNanoSeconds() - masterTime.GetNanoSeconds()
    ));
    std::cout << "Node " << m_nodeId << ": Clock synchronized." << std::endl <<
      "Offset Before Sync: " << m_prevOffsetError << std::endl <<
      "Offset After Sync: " << m_currOffsetError;
  }
}

double PtpNode::getClockError() {
  return m_clockError;
}

int PtpNode::getSentPacketCounter(PtpMessageType_t msgType) {
  return m_sentPacket[msgType];
}

int PtpNode::getReceivedPacketCounter(PtpMessageType_t msgType) {
  return m_receivedPacket[msgType];
}

double PtpNode::getCurrentOffsetError() {
  return m_currOffsetError;
}
