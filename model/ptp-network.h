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
 * This file declares the PTP Network test class.
 *
 */

#ifndef PTP_NETWORK_H
#define PTP_NETWORK_H

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include <vector>
#include <cstdlib>
#include <fstream>
#include "ptp-node.h"
#include "ptp-socket-link.h"

using namespace ns3;

/**
 * \brief IEEE 1588 Test Network Structure
 * 
 * The class maintains the global view of the test environment for IEEE 1588
 * PTP protocol. 
 */
class PTPNetwork {
public:
  /**
   * @brief Construct a new PTPNetwork object
   * 
   * @param users Number of users in the network.
   * @param neighborNode Neighbors of each user in the network,.
   * @param packetSize Default message packet size.
   * @param interPacketInterval Transmission interval of packets
   */
  PTPNetwork (
    const uint32_t users,
    const uint32_t packetSize,
    const Time interPacketInterval,
    const std::string logdir
  );

  /**
   * @brief Add Socket Link to network
   * 
   * @param socketLink 
   */
  void addSocketLink(SocketLink *socketLink);

  void setLogdir(std::string logdir);
  
  /**
   * @brief Add Tx/Rx sockets for simulated network traffic in PTP network
   * 
   * @param txLink 
   * @param rxLink 
   */
  void addTrafficSocket(
    SocketLink *txLink, SocketLink *rxLink
  );

  /**
   * @brief Add PTP Node to network
   * 
   * @param node 
   */
  void addNode(PtpNode *node);

  /**
   * @brief Get the Node By Id object
   * 
   * @param index Index to the network node. 
   * @return PtpNode* 
   */
  PtpNode *getNodeById(uint16_t index);

  /**
   * @brief Set the Local Time At Nodes object
   */
  void setLocalTimeAtNodes();

  /**
   * @brief Callback function when PTP message is received.
   * 
   * @param socket 
   */
  void receivePacket (Ptr<Socket> socket);

  /**
   * @brief Start PTP protocol.
   * 
   * The protocol is started by the master sending SYNC and FOLLOW message to its
   * neighbors.
   */
  void startPTPProtocol();

  /**
   * @brief Send SYNC and FOLLOW packet
   * 
   * The PTP starts with the master timestamp and send SYNC protocol to slave.
   * A FOLLOW message is sent with the timestamp as the payload.
   * 
   * @param sock The network socket link
   * @param eventId The event ID
   */
  void sendSyncFollowPacket(SocketLink *socketLink, int eventId);

  /**
   * @brief Send DREQ packet.
   * 
   * @param socketLink The socket to send the DREQ packet
   * @param eventId 
   */
  void sendDreqPacket(SocketLink *socketLink, int eventId);

  /**
   * @brief Send DRPLY packet.
   * 
   * @param socketLink The socket to send DRPLY packet.
   * @param eventId 
   */
  void sendDrplyPacket(SocketLink *socketLink, int eventId);

  /**
   * @brief Start TCP traffic simulation
   * 
   * @param interval 
   * @param packetSize 
   */
  void startTcpTraffic(Time interval, uint32_t packetSize);

  /**
   * @brief Stop TCP traffic simulation
   */
  void stopTcpTraffic();

  /**
   * @brief Bind the receiving function to sockets binded on receiver of simulated TCP traffic.
   */
  void recvTcpTraffic (Ptr<Socket> socket);

  /**
   * @brief Echo simulated traffic received from TCP socket
   */
  void echoTcpTraffic(
    uint16_t hostId, uint16_t rxNodeId, uint64_t eventId, uint32_t pktLength
  );

  /**
   * @brief Print clock values of a PTP node in the system
   * 
   * @param txIp 
   * @param rxIp 
   * @param txHop 
   * @param msgType 
   * @param dreqAtMaster 
   * @param syncSendTime 
   * @param id 
   */
  void printClockValuesOfNodes(
    Ipv4Address txIp, Ipv4Address rxIp, 
    uint16_t txHop, PtpMessageType_t msgType,
    Time dreqAtMaster, Time syncSendTime, int id
  );

  /**
   * @brief Set the Animation Interface
   * 
   * @param anim 
   * @param offsetCounterId 
   */
  void setAnimationInterface(
    AnimationInterface *anim, int offsetCounterId
  );

  void setSimulationIterations(int iterations);

  void closeLogs();

private:
  int m_iterations; //< Iterations to run

  int m_eventId;  //< Global PTP event Id.
  int m_eventCounterId; //< current event counter index.
  const uint32_t m_users;  //< Number of users in the network
  uint16_t m_masterIndex;  ///< Index of the master node

  std::vector<SocketLink *> m_socketLinks; //< Established sockets for PTP message transmission.
  std::vector<PtpNode *> m_nodes; //< PTP clock nodes.

  Time m_globalTime; //< Simulator global time
  const uint32_t m_packetSize; //< Packet Size
  const Time m_interPacketInterval; //< Synchronization Interval

  std::vector<SocketLink *> m_txTrafficLinks; //< Sockets on traffic generator node sending simulated traffic.
  std::vector<SocketLink *> m_rxTrafficLinks; //< Sockets on PTP nodes receiving simulated traffic.
  Time m_trafficInterval; //< Interval to send TCP packets to simulate network traffic
  uint32_t m_trafficPacketSize; //< Size of each TCP packet for network traffic simulation
  bool m_simulatingTraffic; //< Whether simulating the traffic at the moment

  AnimationInterface *m_anim; //< Animation interface for logging
  int m_ptpOffsetCounterId; //< Animation interface clock offset counter ID

  std::string m_logdir;
  std::vector<std::ofstream *> m_fileStreams;
};

#endif /* PTP_NETWORK_H */
