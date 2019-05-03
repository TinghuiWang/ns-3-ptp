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
 * Test IEEE 1588 PTP in wireless environment.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ptp-network.h"
#include "ptp-node.h"

using namespace ns3;

int main(int argc, char **argv) {
  /* Default parameters for the test */
  std::string phyMode ("DsssRate1Mbps");
  double rss = -93; // -dBm
  uint32_t packetSize = 1024; // Bytes
  uint8_t interval = 5; // nanoseconds
  uint32_t nUsers = 6; // Number of users

  /* Setup Command Line Arguments */
  CommandLine cmd;
  cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue("rss", "received signal strength", rss);
  cmd.AddValue("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue("interval", "interval (seconds) between packets", interval);
  cmd.AddValue("users", "Number of receivers", nUsers);
  cmd.Parse(argc, argv);

  // Convert to time object
  Time interPacketInterval = NanoSeconds(interval);
  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
    StringValue("2200"));
  // Turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
    StringValue("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
    StringValue (phyMode));

  // Create nodes
  // Not only create `nUsers` nodes for PTP terminals,
  // but add the `nUsers + 1`th node for traffic generation
  NodeContainer nodes;
  nodes.Create(nUsers + 1);

  // The following helpers will put together the wifi NICs
  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b); // OFDM @ 2.4GHz

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // The default error rate model is ns3::NistErrorRateModel

  // This is one parameter that matters when using FixedRssLossModel
  // Set to 0; otherwise gain will be added
  wifiPhy.Set("RxGain", DoubleValue(0));

  // ns-3 supports RadioTap and Prism tracing extensions for 802.11g
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",
    DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, set to adhoc mode, and disable rate control
  WifiMacHelper wifiMac;
  wifiMac.SetType("ns3::AdhocWifiMac", "QosSupported", BooleanValue(false));
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
    "DataMode",StringValue(phyMode), "ControlMode", StringValue(phyMode));

  // Create the net devices
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

  // Note that with FixedRssLossModel, the positions below are not used for
  // received signal strength. However they are required for YansWifiChannelHelper.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc =
    CreateObject<ListPositionAllocator> ();

  for (uint32_t n = 1; n <= nUsers; n++)
  {
    positionAlloc->Add(Vector(5.0, 5.0*n, 0.0));
  }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  // Instantiate all the nodes as internet
  InternetStackHelper internet;
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  ipv4.Assign(devices);
  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  TypeId tcpId = TypeId::LookupByName("ns3::TcpSocketFactory");

  // List of Ipv4 Address
  //int hopOfNode[ 3 ] ={ 0, 1, 1 };
  char ipv4Add[7][12] = { "10.1.1.1", "10.1.1.2" , "10.1.1.3",
                          "10.1.1.4", "10.1.1.5" , "10.1.1.6",
                          "10.1.1.7" };

  uint16_t connectToPortAtNode[6] = { 100, 200, 300, 400, 500, 600};
  uint16_t hopOfNode[6] = {0, 1, 2, 3, 4, 5};
  uint16_t masterOfNode[6] = {0, 0, 1, 2, 3, 4};
  // int socketIndex[7]; // What is this?

  int neighborList[6][4] = {
    { 1, -1},    // Node 0
    { 0, 2, -1}, // Node 1
    { 1, 3, -1}, // Node 2
    { 2, 4, -1}, // Node 3
    { 3, 5, -1}, // Node 4
    { 4, -1}     // Node 5
  };

  uint16_t neighborPort[6][3] = {
    { 200 },
    { 100, 300 },
    { 201, 400 },
    { 301, 500 },
    { 401, 600 },
    { 501 }
  };

  // Store static PTP nodes
  std::vector<PtpNode *> staticNodes(nUsers);
  // Store IPv4 address of each node
  std::vector<Ipv4Address> ipv4Address(nUsers + 1);
  // Socket to neighbor node
  std::vector<std::vector<Ptr<Socket>>> neighbor(nUsers);
  // List of socket links established in the network
  std::vector<SocketLink *> socketLinks(10);

  uint16_t i, j, socket_count=0, srcPort, dstPort;

  // create Ipv4 address 
  for( i=0; i < nUsers + 1;i++){
    ipv4Address[i] = Ipv4Address(ipv4Add[i]);
  }

  // Create neighbor nodes
  PTPNetwork ptpTest(nUsers, packetSize, interPacketInterval);
  
  // socketIndex[0] = -1;
  for(i = 0; i < nUsers; i++) {
    j = 0;
    staticNodes[i] = new PtpNode(i, masterOfNode[i], hopOfNode[i], ipv4Address[i]);
    while(neighborList[i][j] != -1 && j < 4) {
      // Create an UDP Socket on node i and add it to neighbor socket vectors.
      neighbor[i].push_back(Socket::CreateSocket(nodes.Get(i), tid));
      // The port is (i + 1) * 100 + j
      srcPort = connectToPortAtNode[i] + j;
      // Bind to source IP and source port
      neighbor[i][j]->Bind(InetSocketAddress(ipv4Address[i], srcPort));
      // Connect the socket to neighbor identified in neighborList[i][j]
      neighbor[i][j]->Connect(
        InetSocketAddress(ipv4Address[neighborList[i][j]], neighborPort[i][j])
      );
      // Print information to console
      std::cout << "[Socket] txNode: " << i << " " << 
        ipv4Address[i] << ":" << srcPort << " --> " << 
        "DstNode: " << neighborList[i][j] << " " <<
        ipv4Address[neighborList[i][j]] << ":" << neighborPort[i][j] << 
        std::endl;
      // Set Receive Callback for each socket
      neighbor[i][j]->SetRecvCallback(
        MakeCallback(&PTPNetwork::receivePacket, &ptpTest)
      );
      // Create socket Link
      socketLinks[socket_count] = new SocketLink(
        i, neighborList[i][j], 
        ipv4Address[i], srcPort, 
        ipv4Address[neighborList[i][j]], neighborPort[i][j],
        neighbor[i][j]
      );
      // Add Neighbor
      staticNodes[i]->addNeighbor(
        neighborList[i][j], socketLinks[socket_count]
      );
      // Add to PTP Network
      ptpTest.addSocketLink(socketLinks[socket_count]);
      
      socket_count++;
      j++;
    }
    ptpTest.addNode(staticNodes[i]);
    // Create Socket for simulating network traffic
    srcPort = i * 1000;
    dstPort = nUsers * 1000 + i;
    Ptr<Socket> rxTrafficSocket = Socket::CreateSocket(nodes.Get(i), tcpId);
    rxTrafficSocket->Bind(InetSocketAddress(ipv4Address[i], srcPort));
    rxTrafficSocket->Connect(InetSocketAddress(ipv4Address[nUsers], dstPort));
    rxTrafficSocket->SetRecvCallback(&PTPNetwork::recvTcpTraffic, &ptpTest);
    Ptr<Socket> txTrafficSocket = Socket::CreateSocket(nodes.Get(nUsers), tcpId);
    txTrafficSocket->Bind(InetSocketAddress(ipv4Address[nUsers], dstPort));
    txTrafficSocket->Connect(InetSocketAddress(ipv4Address[i], srcPort));
    txTrafficSocket->SetRecvCallback(&PTPNetwork::recvTcpTraffic, &ptpTest);
    ptpTest.addTrafficSocket(
      new SocketLink(
        nUsers, i, ipv4Address[nUsers], dstPort, ipv4Address[i], srcPort,
        txTrafficSocket
      ),
      new SocketLink(
        i, nUsers, ipv4Address[i], srcPort, ipv4Address[nUsers], dstPort,
        rxTrafficSocket
      )
    );
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  // Pcap tracing
  wifiPhy.EnablePcap ("ptp-wifi-broadcast", devices);

  AnimationInterface anim("ptp-test.xml");
  anim.SetConstantPosition(nodes.Get(0), 0.0, 15.0);
  anim.SetConstantPosition(nodes.Get(1), 4.0, 15.0);
  anim.SetConstantPosition(nodes.Get(2), 8.0, 15.0);
  anim.SetConstantPosition(nodes.Get(3), 12.0, 15.0);
  anim.SetConstantPosition(nodes.Get(4), 16.0, 15.0);
  anim.SetConstantPosition(nodes.Get(5), 20.0, 15.0);
  anim.SetConstantPosition(nodes.Get(6), 10.0, 5.0);
  anim.EnablePacketMetadata(true);
  int clkOffsetCounterId = anim.AddNodeCounter(
    "offset_error", AnimationInterface::DOUBLE_COUNTER
  );
  ptpTest.setAnimationInterface(
    &anim, clkOffsetCounterId
  );
  ptpTest.setSimulationIterations(10);
  
  Simulator::ScheduleWithContext(
    nUsers, Seconds(1.0), &PTPNetwork::startTcpTraffic, &ptpTest,
    Seconds(1.0), 1024
  );

  Simulator::ScheduleWithContext(
    neighbor[0][0]->GetNode()->GetId(),
    Seconds(1.0),
    &PTPNetwork::startPTPProtocol,
    &ptpTest
  );

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
