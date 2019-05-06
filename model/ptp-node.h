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
 *
 */

#ifndef PTP_NODE_H
#define PTP_NODE_H

#include "ns3/core-module.h"
#include "ns3/ipv4-address.h"
#include "ptp-message.h"
#include "ptp-socket-link.h"

using namespace ns3;

typedef enum {
  INACTIVE = 0,
  ACTIVE,
  WAITING,
  SYNCED
} NodeState_t;

class PtpNode {
public:

  PtpNode(
    const uint16_t id,
    const uint16_t masterId,
    const uint16_t hop,
    const Ipv4Address ipv4Address
  );

  /**
   * @brief Get node hop
   */
  uint16_t getNodeHop();

  /**
   * @brief Get node ID
   */
  uint16_t getNodeId();

  /**
   * @brief Get master node ID
   */
  uint16_t getMasterId();

  /**
   * @brief Get the Ipv4 Address of the node
   * 
   * @return Ipv4Address 
   */
  Ipv4Address getIpv4Address();

  /**
   * @brief Set node as master
   */
  void setGlobalMaster();
  
  /**
   * @brief Is the node selected to provide global master clock
   * 
   * @return true 
   * @return false 
   */
  bool isGlobalMaster();

  /**
   * @brief Update node state
   * 
   * @param s state of the node (INACTIVE, ACTIVE, WAITING or SYNC)
   */
  void setState(NodeState_t s);

  /**
   * @brief Get the state of the node
   * 
   * @return TNodeState state of the node (INACTIVE, ACTIVE, WAITING or SYNC)
   */ 
  NodeState_t getState();

  /**
   * @brief Set the Initial Time object
   * 
   * @param time 
   */
  void setInitialTime(Time time);

  /**
   * @brief Set Node Local Time
   * This function is called after happening of any event in the network
   * 
   * @param time 
   */
  void setLocalTime(Time simulatorTime);

  /**
   * @brief Get local time of the node
   * 
   * @return Time 
   */
  Time getLocalTime();

  /**
   * @brief Get SYNC message timestamp at master
   * 
   * @return Time 
   */
  Time getSyncTimeAtMaster();

  /**
   * @brief Set the Sync Time At Master
   * 
   * @param time 
   */
  void setSyncTimeAtMaster(Time time);

  /**
   * @brief Get the Dreq Time At Master
   * 
   * @return Time 
   */
  Time getDreqTimeAtMaster();

  /**
   * @brief Set the Dreq Time At Master
   * 
   * @param time 
   */
  void setDreqTimeAtMaster(Time time);

  /**
   * @brief Get the Sync Recv Time
   * 
   * @return Time 
   */
  Time getSyncRecvTime();

  /**
   * @brief Set the Sync Recv Time
   * 
   * @param time 
   */
  void setSyncRecvTime(Time time);

  /**
   * @brief Get the Dreq Send Time
   * 
   * @return Time 
   */
  Time getDreqSendTime();

  /**
   * @brief Set the Dreq Send Time
   * 
   * @param time 
   */
  void setDreqSendTime(Time time);

  /**
   * @brief Get SYNC message send time
   * 
   * @return Time 
   */
  Time getSyncSendTimeStamp(uint16_t nodeId);

  /**
   * @brief Create timestamp for SYNC message
   */
  void setSyncSendTimeStamp(Time time, uint16_t nodeId);

  /**
   * @brief Get the Dreq Send Time
   * 
   * @return Time 
   */
  Time getDreqRecvTimeStamp(uint16_t nodeId);

  /**
   * @brief Set the Dreq Send Time
   * 
   * @param time 
   */
  void setDreqRecvTimeStamp(Time time, uint16_t nodeId);

  /**
   * @brief Get number of neighbors of the current node
   * 
   * @return int 
   */
  int getNumNeighbors();

  /**
   * @brief Add neighbor node
   * 
   * @param nodeId 
   */
  void addNeighbor(
    uint16_t nodeId,
    SocketLink *txSocket
  );

  /**
   * @brief Get the Tx Socket Link
   * 
   * @param index 
   * @return SocketLink* 
   */
  SocketLink *getTxSocket(int index);

  /**
   * @brief Get the Tx Socket By Node Id
   * 
   * @param nodeId 
   * @return SocketLink* 
   */
  SocketLink *getTxSocketByNodeId(uint16_t nodeId);

  /**
   * @brief Increase sent packet counter (by message type)
   * 
   * @param msgType 
   */
  void incrementSentPacketCounter(PtpMessageType_t msgType);

  /**
   * @brief Increase received packet counter (by message type)
   * 
   * @param msgType 
   */
  void increaseReceivedPacketCounter(PtpMessageType_t msgType);

  /**
   * @brief Calculate time offset
   * 
   * @param masterTime reference master clock
   */
  void calculateOffset(Time masterTime);
  
  /**
   * @brief Get number of packets sent of message type `msgType`.
   * 
   * @param msgType 
   * @return int 
   */
  int getSentPacketCounter(PtpMessageType_t msgType);

  /**
   * @brief Get number of packets received of message type `msgType`.
   * 
   * @param msgType 
   * @return int 
   */
  int getReceivedPacketCounter(PtpMessageType_t msgType);

  /**
   * @brief Get the Clock Error
   * 
   * @return double 
   */
  double getClockError();

  double getPreviousOffsetError();

  double getCurrentOffsetError();

  uint64_t getNewSyncId(uint16_t nodeId);

  void setPtpSyncId(PtpMessageType_t msgType, uint64_t syncId);

  uint64_t getPtpSyncId(PtpMessageType_t msgType);

private:
  Time m_localTime; // Local time
  Time m_simulatorTime; // Global Simulation Time

  NodeState_t m_nodeState; //< PTP Node State: Inactive, Active, Waiting, Synced

  /* Time stamps used with current node as clock slave */
  /* Timestamp recorded at corresponding Master */
  Time m_syncTimeAtMaster; //< The time stamp when SYNC message is sent
  Time m_dreqTimeAtMaster; //< The time stamp when DREQ message is sent
  /* Local time stamp */
  Time m_syncRecvTime; //< The time stamp when SYNC message is received
  Time m_dreqSendTime; //< The time stamp when the slave node sends DREQ message
  std::vector<uint64_t> m_ptpMsgSyncId;

  /* Time stamps used with current node as clock master for other slave neighbors */
  std::vector<Time> m_syncSendTimeStamps; //< The time stamp when SYNC is sent to each neighbor.
  std::vector<Time> m_dreqRecvTimeStamps; //< The time stamp when DREQ is received from each neighbor.
  std::vector<uint64_t> m_masterSyncId;

  Time m_offset; //< Local offset to the global time of the simulator

  /* Set during initialization */
  bool m_isGlobalMaster; //< Is global master clock node
  const uint16_t m_nodeId; //< Node ID
  const uint16_t m_masterId; //< The ID of the node that provides the master clock for the current node
  const uint16_t m_hopNum; //< Number of hops away from master.

  /* Node configuration */
  const Ipv4Address m_nodeIpv4Address; //< Node IPv4 Address

  /* Neighbor Information */
  std::vector<SocketLink *> m_sockets;

  /* Statistics */
  double m_clockError;
  double m_prevOffsetError;
  double m_currOffsetError;

  /* Statistics */
  std::vector<uint16_t> m_neighbors; ///< Neighbors of current node identified by nodeID
  std::vector<int> m_sentPacket; ///< Number of each type PTP messages sent (indexed by message type: SYNC, FOLLOW, DREQ and DRPLY).
  std::vector<int> m_receivedPacket;// vector indexed by packet type(Sync, Follow, Dreq, Drply) and stores num of packets received
  std::vector<int> m_overheardPacket;// vector indexed by packet type(Sync, Follow, Dreq, Drply) and stores num of packets overheard and ignored
};

#endif /* PTP_NODE_H */
