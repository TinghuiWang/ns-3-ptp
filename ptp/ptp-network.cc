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
 * This file implements the functions of PTPNetwork test class
 */

#include "ns3/core-module.h"
#include "ptp-network.h"
#include "ptp-socket-link.h"
#include "ptp-message.h"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace ns3;

PTPNetwork::PTPNetwork(
  const uint32_t users,
  const uint32_t packetSize,
  const Time interPacketInterval
) : m_users(users),
    m_packetSize(packetSize),
    m_interPacketInterval(interPacketInterval)
    {
      m_iterations = 1;
      m_masterIndex = 0;
      m_eventId = 0;
      m_eventCounterId = 0;
    }

void PTPNetwork::addSocketLink(SocketLink *socketLink) {
  m_socketLinks.push_back(socketLink);
}

void PTPNetwork::addNode(PtpNode *node) {
  m_nodes.push_back(node);
}

PtpNode *PTPNetwork::getNodeById(uint16_t nodeId) {
  return m_nodes[nodeId];
}

void PTPNetwork::receivePacket(Ptr<Socket> socket) {
  // Record simulator time when packet is received at socket.
  m_globalTime = NanoSeconds(Simulator::Now());
  m_nodes[m_masterIndex]->setLocalTime(m_globalTime);

  uint16_t i = 0;
  //int numNeighbor;

  uint16_t hostId, senderId;
  PtpNode *hostNode;
  PtpNode *senderNode;
  // Ptr<Socket> socketToNeighbor;
  SocketLink *socketLink;

  // Acquire packets from socket
  Ptr<Packet> pktReceived = socket->Recv();
  // uint32_t pktLength = pktReceived->GetSize();
  PtpMessage_t *ptpMessage = (PtpMessage_t *) std::malloc(sizeof(PtpMessage_t));
  pktReceived->CopyData((uint8_t *) ptpMessage, sizeof(PtpMessage_t));

  // Now, we need to handle response to the packets
  // First, find from which neighbor this message comes from
  i = 0;
  while(!(m_socketLinks[i]->getSocket() == socket)) {
    i++;
  }
  socketLink = m_socketLinks[i];
  // Host is the node that receives the packet
  hostId = socketLink->getHostId();
  hostNode = this->getNodeById(hostId);
  // Source of the PTP message should be acquired from the message
  senderId = ptpMessage->txNodeId;
  senderNode = this->getNodeById(senderId);

  setLocalTimeAtNodes();

  // Read Contents from the packet and prepare response
  if(ptpMessage->messageType == SYNC) {
    // store SYNC receive time and wait for follow up
    hostNode->setSyncRecvTime(hostNode->getLocalTime());
    hostNode->increaseReceivedPacketCounter(ptpMessage->messageType);
    hostNode->setState(ACTIVE);
    printClockValuesOfNodes(
      senderNode->getIpv4Address(), hostNode->getIpv4Address(),
      senderNode->getNodeHop(), ptpMessage->messageType,
      senderNode->getDreqSendTime(), senderNode->getSyncSendTimeStamp(hostId),
      ptpMessage->eventId
    );
  } else if(ptpMessage->messageType == FOLLOW) {
    // store SYNC send time and send DREQ
    hostNode->setSyncTimeAtMaster(NanoSeconds(ptpMessage->timeStamp));
    hostNode->increaseReceivedPacketCounter(ptpMessage->messageType);
    // Delay update local time until the end of sequence
    printClockValuesOfNodes(
      senderNode->getIpv4Address(), hostNode->getIpv4Address(),
      senderNode->getNodeHop(), ptpMessage->messageType,
      senderNode->getDreqSendTime(), senderNode->getSyncSendTimeStamp(hostId),
      ptpMessage->eventId
    );
    // Schedule DREQ Packet Send
    m_eventId++;
    Simulator::Schedule(
      NanoSeconds(0), 
      &PTPNetwork::sendDreqPacket,
      this, socketLink, m_eventId
    );
  } else if(ptpMessage->messageType == DREQ) {
    // Time stamp, and then send DRPLY
    hostNode->setDreqRecvTimeStamp(hostNode->getLocalTime(), ptpMessage->txNodeId);
    hostNode->increaseReceivedPacketCounter(DREQ);
    printClockValuesOfNodes(
      senderNode->getIpv4Address(), hostNode->getIpv4Address(),
      senderNode->getNodeHop(), ptpMessage->messageType,
      senderNode->getDreqSendTime(), senderNode->getSyncSendTimeStamp(hostId),
      ptpMessage->eventId
    );
    Simulator::Schedule(
      NanoSeconds(0),
      &PTPNetwork::sendDrplyPacket,
      this, socketLink, ptpMessage->eventId
    );
  } else if(ptpMessage->messageType == DRPLY) {
    // Update clock and mark SYNCED
    hostNode->setDreqTimeAtMaster(NanoSeconds(ptpMessage->timeStamp));
    hostNode->increaseReceivedPacketCounter(DRPLY);
    hostNode->setState(SYNCED);
    // Update offset and error calculation
    hostNode->calculateOffset(
      this->getNodeById(m_masterIndex)->getLocalTime()
    );
    m_anim->UpdateNodeCounter(
      m_ptpOffsetCounterId, 
      hostNode->getNodeId(), 
      hostNode->getCurrentOffsetError()
    );
    printClockValuesOfNodes(
      senderNode->getIpv4Address(), hostNode->getIpv4Address(),
      senderNode->getNodeHop(), ptpMessage->messageType,
      senderNode->getDreqSendTime(), senderNode->getSyncSendTimeStamp(hostId),
      ptpMessage->eventId
    );
    for(int i = 0; i < hostNode->getNumNeighbors(); i++) {
      SocketLink *sockToNeighbor = hostNode->getTxSocket(i);
      if(sockToNeighbor->getDstId() != senderId) {
        Simulator::Schedule(
          NanoSeconds(0),
          &PTPNetwork::sendSyncFollowPacket, this, 
          sockToNeighbor, m_eventId
        );
        m_eventId++;
      }
    }
  } else {
    std::cerr << "[PTPNetwork::receivePacket] Error: PTP message with " << 
      "invalid message type received." << std::endl;
  }
}

void PTPNetwork::startPTPProtocol() {
  // Get master node and set master node status to "SYNCED"
  PtpNode *master = this->getNodeById(m_masterIndex);
  master->setState(SYNCED);

  // Schedule SEND and FOLLOW message
  for(int i = 0; i < master->getNumNeighbors(); i++) {
    SocketLink *sockToNeighbor = master->getTxSocket(i);
    Simulator::Schedule(
      m_interPacketInterval, 
      &PTPNetwork::sendSyncFollowPacket, this, 
      sockToNeighbor, m_eventId
    );
  }
  m_eventId++;
  m_iterations--;
  if(m_iterations != 0) {
    Simulator::Schedule(
      Seconds(1.0),
      &PTPNetwork::startPTPProtocol, this
    );
  }
}

void PTPNetwork::sendSyncFollowPacket(
  SocketLink *socketLink, int eventId
) {
  uint16_t txId = socketLink->getHostId();
  uint16_t rxId = socketLink->getDstId();
  PtpNode *txNode = m_nodes[txId];
  Ptr<Socket> sock = socketLink->getSocket();

  PtpMessage_t *msgSync = (PtpMessage_t *) std::malloc(sizeof(PtpMessage_t));
  msgSync->txNodeId = txNode->getNodeId();
  msgSync->txNodeHop = txNode->getNodeHop();
  msgSync->messageType = SYNC;
  msgSync->eventId = eventId;
  msgSync->timeStamp = txNode->getSyncSendTimeStamp(rxId).GetNanoSeconds();

  // Send Sync Packet
  Ptr<Packet> pktSync = Create<Packet>((uint8_t *) msgSync, sizeof(PtpMessage_t));
  sock->Send(pktSync);

  // Timestamping
  m_globalTime = NanoSeconds(Simulator::Now());
  setLocalTimeAtNodes();
  txNode->setSyncSendTimeStamp(txNode->getLocalTime(), rxId);
  std::cout << "sending SYNC packet" << std::endl;
  txNode->incrementSentPacketCounter(SYNC);

  // constructing FOLLOW-UP packet
  PtpMessage_t *msgFollow = (PtpMessage_t *) std::malloc(sizeof(PtpMessage_t));
  msgFollow->txNodeId = txNode->getNodeId();
  msgFollow->txNodeHop = txNode->getNodeHop();
  msgFollow->messageType = FOLLOW;
  msgFollow->eventId = eventId;
  msgFollow->timeStamp = txNode->getSyncSendTimeStamp(rxId).GetNanoSeconds();

  // Send FOLLOW UP Packet
  Ptr<Packet> pktFollow = Create<Packet>((uint8_t *) msgFollow, sizeof(PtpMessage_t));
  sock->Send(pktFollow);

  // TimeStamping?
  m_globalTime = NanoSeconds(Simulator::Now());
  setLocalTimeAtNodes();
  std::cout << "sending FOLLOW packet" << std::endl;
  txNode->incrementSentPacketCounter(FOLLOW);

  // Free both malloced buffer
  std::free(msgSync);
  std::free(msgFollow);
}

void PTPNetwork::sendDreqPacket(SocketLink *socketLink, int eventId) {
  PtpNode *txNode = this->getNodeById(socketLink->getHostId());
  // Prepare DREQ message
  PtpMessage_t *msgDreq = (PtpMessage_t *) std::malloc(sizeof(PtpMessage_t));
  msgDreq->txNodeId = txNode->getNodeId();
  msgDreq->txNodeHop = txNode->getNodeHop();
  msgDreq->messageType = DREQ;
  msgDreq->eventId = eventId;
  msgDreq->timeStamp = 0;
  // Send packet
  Ptr<Packet> pktDreq = Create<Packet>((uint8_t *) msgDreq, sizeof(PtpMessage_t));
  socketLink->getSocket()->Send(pktDreq);

  // Time Stamp
  txNode->setState(WAITING);
  m_globalTime = NanoSeconds(Simulator::Now());
  setLocalTimeAtNodes();
  txNode->setDreqSendTime(txNode->getLocalTime());
  std::cout << "sending DREQ packet" << std::endl;
  txNode->incrementSentPacketCounter(DREQ);

  // Free malloced buffer
  free(msgDreq);
}

void PTPNetwork::sendDrplyPacket(SocketLink *socketLink, int eventId) {
  PtpNode *txNode = this->getNodeById(socketLink->getHostId());
  PtpNode *rxNode = this->getNodeById(socketLink->getDstId());
  // Prepare DREQ message
  PtpMessage_t *msgDrply = (PtpMessage_t *) std::malloc(sizeof(PtpMessage_t));
  msgDrply->txNodeId = txNode->getNodeId();
  msgDrply->txNodeHop = txNode->getNodeHop();
  msgDrply->messageType = DRPLY;
  msgDrply->eventId = eventId;
  msgDrply->timeStamp = txNode->getDreqRecvTimeStamp(rxNode->getNodeId()).GetNanoSeconds();
  // Send packet
  Ptr<Packet> pktDrply = Create<Packet>((uint8_t *) msgDrply, sizeof(PtpMessage_t));
  socketLink->getSocket()->Send(pktDrply);

  // Time Stamp
  m_globalTime = NanoSeconds(Simulator::Now());
  setLocalTimeAtNodes();
  std::cout << "sending DRPLY packet" << std::endl;
  txNode->incrementSentPacketCounter(DRPLY);

  // Free malloced buffer
  free(msgDrply);
}

void PTPNetwork::setLocalTimeAtNodes() {
  for(uint32_t i = 0; i < m_nodes.size(); i++) {
    m_nodes[i]->setLocalTime(m_globalTime);
  }
}

void PTPNetwork::printClockValuesOfNodes(
  Ipv4Address txIp, Ipv4Address rxIp, 
  uint16_t txHop, PtpMessageType_t msgType,
  Time dreqAtMaster, Time syncSendTime, int id
) {
  std::string strMsgType;
  switch(msgType) {
    case SYNC:
      strMsgType = "SYNC";
      break;
    case FOLLOW:
      strMsgType = "FOLLOW UP";
      break;
    case DREQ:
      strMsgType = "DELAY REQUEST";
      break;
    case DRPLY:
      strMsgType = "DELAY REPLY";
      break;
    default:
      strMsgType = "UNKNOWN";
      break;
  }

  std::cout << "----------------------------------------- " << std::endl;
  std::cout << " Sender: " << txIp << " [hop: " << txHop << "]" << std::endl;
  std::cout << " Receiver: " << rxIp << std::endl;
  std::cout << " Message Type: " << strMsgType << " , ID: " << id << std::endl;
  std::cout << " TxNode Timestamps: " << 
    std::setw(12) << dreqAtMaster.GetNanoSeconds() << " [DREQ], " <<
    std::setw(12) << syncSendTime.GetNanoSeconds() << " [SYNC]" << std::endl;
  std::cout << std::endl;
  std::cout << std::setw(6) << "NodeId" << " => " <<
    std::setw(12) << "ClockDev." << " => " <<
    std::setw(12) << "ErrBeforeSync" << " => " <<
    std::setw(12) << "ErrAfterSync" << " => " <<
    std::setw(12) << "Time" << " => " <<
    std::setw(12) << "State" << " => " <<
    std::setw(12) << "Curr.Offset" << " => " <<
    std::setw(12) << "Calc.Offset" << " => " <<
    std::setw(12) << "Sync Time" << " => " << 
    std::setw(17) << "SYNC" << " => " <<
    std::setw(17) << "FOLLOW-UP" << " => " <<
    std::setw(17) << "DELAY-REQ" << " => " <<
    std::setw(17) << "DELAY-RPLY" << std::endl;
  PtpNode *masterNode = this->getNodeById(m_masterIndex);
  for(uint32_t j = 0; j < m_users; j++) {
    PtpNode *node = this->getNodeById(j);
    Time clockTime = node->getLocalTime();
    NodeState_t nodeState = node->getState();
    std::string strNodeState;
    switch(nodeState) {
      case INACTIVE:
        strNodeState = "INACTIVE";
        break;
      case ACTIVE:
        strNodeState = "ACTIVE";
        break;
      case WAITING:
        strNodeState = "WAITING";
        break;
      case SYNCED:
        strNodeState = "SYNCED";
        break;
      default:
        strNodeState = "UNKNOWN";
        break;
    }
    // The offset between the local time of a node w.r.t. to the master clock.
    Time presentOffset = NanoSeconds(
      node->getLocalTime().GetNanoSeconds() - 
      masterNode->getLocalTime().GetNanoSeconds()
    );
    std::cout << std::setw(6) << j << "    " <<
      std::setw(12) << node->getClockError() << "    " <<
      std::setw(12) << "EBS N/A" << "    " <<
      std::setw(12) << "EAS N/A" << "    " <<
      std::setw(12) << node->getLocalTime().GetNanoSeconds() << "    " <<
      std::setw(12) << strNodeState << "    " <<
      std::setw(12) << presentOffset.GetNanoSeconds() << "    " <<
      std::setw(12) << "N/A" << "    " <<
      std::setw(12) << "N/A" << "    " << 
      std::setw(2) << node->getSentPacketCounter(SYNC) << " [tx] , " <<
      std::setw(2) << node->getReceivedPacketCounter(SYNC) << " [rx]    " <<
      std::setw(2) << node->getSentPacketCounter(FOLLOW) << " [tx] , " <<
      std::setw(2) << node->getReceivedPacketCounter(FOLLOW) << " [rx]    " <<
      std::setw(2) << node->getSentPacketCounter(DREQ) << " [tx] , " <<
      std::setw(2) << node->getReceivedPacketCounter(DREQ) << " [rx]    " <<
      std::setw(2) << node->getSentPacketCounter(DRPLY) << " [tx] , " <<
      std::setw(2) << node->getReceivedPacketCounter(DRPLY) << " [rx]    " <<
      std::endl;;
  }
  std::cout << "----------------------------------------- " << std::endl;
  std::cout << std::endl << std::endl;
}

void PTPNetwork::setAnimationInterface(
  AnimationInterface *anim, int offsetCounterId
) {
  m_anim = anim;
  m_ptpOffsetCounterId = offsetCounterId;
}

void PTPNetwork::setSimulationIterations(int iterations) {
  m_iterations = iterations;
}
