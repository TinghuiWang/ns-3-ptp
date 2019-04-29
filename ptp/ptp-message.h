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
 * This file defines the types for PTP messages.
 *
 */

#ifndef PTP_MESSAGE_H
#define PTP_MESSAGE_H

#include "ns3/core-module.h"
#include <cstdlib>

using namespace ns3;

/**
 * @brief Decleare enumeration type for PTP Message Types
 * SYNC: Initial synchronization message
 * FOLLOW: Follow-up message with master timestamp
 * DREQ: Delay Request message
 * DRPLY: Delay Response message with master timestamp
 */
typedef enum {
  SYNC = 0,
  FOLLOW,
  DREQ,
  DRPLY
} PtpMessageType_t;

/**
 * @brief Simulated information needed in PTP message.
 */
typedef struct PtpMessage {
    uint16_t txNodeId;
    uint16_t txNodeHop;
    PtpMessageType_t messageType;
    int eventId;
    int64_t timeStamp;
} PtpMessage_t;

#endif
