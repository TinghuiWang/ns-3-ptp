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
 * Test IEEE 1588 PTP in wired ethernet situation with csma models.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-module.h"
#include "ns3/netanim-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ptp-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PTP_CSMA_Example");

int main(int argc, char **argv) {
  // LogComponentEnable("PtpNetwork", LOG_LEVEL_INFO);
  // Constructing the topology
  std::string bandwidth ("1Gb/s");
  float delay = 10;
  float utilization = 0.01; 
  uint32_t packetSize = 1024; // Bytes
  uint64_t interval = 50000000; // nanoseconds
  uint32_t nUsers = 6; // Number of users
  std::string logdir ("");

  /* Setup Command Line Arguments */
  CommandLine cmd;
  cmd.AddValue("bandwidth", "Bandwidth of CSMA links", bandwidth);
  cmd.AddValue("delay", "Transmission delay of each CSMA link", delay);
  cmd.AddValue("utilization", "Simulated utilization of CSMA links", utilization);
  cmd.AddValue("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue("interval", "interval (seconds) between packets", interval);
  cmd.AddValue("users", "Number of receivers", nUsers);
  cmd.AddValue("logdir", "Directory to write statistics to", logdir);
  cmd.Parse(argc, argv);

  // Convert to time object
  Time interPacketInterval = NanoSeconds(interval);

  ns3::PacketMetadata::Enable ();

  // Create nodes
  // Not only create `nUsers` nodes for PTP terminals,
  // but add the `nUsers + 1`th node for traffic generation
  std::stringstream createNodeInfo;
  createNodeInfo << "Create " << nUsers << " terminal nodes.";
  NS_LOG_INFO(createNodeInfo.str());
  NodeContainer nodes;
  nodes.Create(nUsers + 1);

  NodeContainer csmaSwitches;
  csmaSwitches.Create(nUsers - 1);

  // Creating network topology
  std::stringstream networkTopologyInfo;
  networkTopologyInfo << "Create CSMA links of " << bandwidth << "bps and " <<
    delay << " ms.";
  NS_LOG_INFO(networkTopologyInfo.str());

  // CSMA Helper
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", DataRateValue(bandwidth));
  csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));

  // Create the CSMA links, from each termial to their corresponding switches
  std::vector<NetDeviceContainer *> switchDeviceContainers;
  NetDeviceContainer link;
  NetDeviceContainer terminalDevices;
  for(uint32_t i = 0; i < nUsers - 1; i++) {
    switchDeviceContainers.push_back(new NetDeviceContainer);
    if (i == 0) {
      link = csma.Install(
        NodeContainer(nodes.Get(i), csmaSwitches.Get(i))
      );
      switchDeviceContainers[i]->Add(link.Get(1));
      terminalDevices.Add(link.Get(0));
    } else {
      link = csma.Install(
        NodeContainer(csmaSwitches.Get(i-1), csmaSwitches.Get(i))
      );
      switchDeviceContainers[i-1]->Add(link.Get(0));
      switchDeviceContainers[i]->Add(link.Get(1));
    }
    link = csma.Install(
      NodeContainer(nodes.Get(i + 1), csmaSwitches.Get(i))
    );
    terminalDevices.Add(link.Get(0));
    switchDeviceContainers[i]->Add(link.Get(1));
    link = csma.Install(
      NodeContainer(nodes.Get(nUsers), csmaSwitches.Get(i))
    );
    switchDeviceContainers[i]->Add(link.Get(1));
  }
  terminalDevices.Add(link.Get(0));

  // Install Bridge Helper
  BridgeHelper bridge;
  for(uint32_t i = 0; i < nUsers-1; i++) {
    bridge.Install(
      csmaSwitches.Get(i), *(switchDeviceContainers[i])
    );
  }

  // Add Internet stack to terminals
  InternetStackHelper internet;
  internet.Install(nodes);

  // Add IP Address for each PTP terminal nodes
  Ipv4AddressHelper ipv4Helper;
  ipv4Helper.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer deviceIpv4InterfaceContainer;
  deviceIpv4InterfaceContainer = ipv4Helper.Assign(terminalDevices);
  // std::vector<Ipv4Address> ipv4Addresses;
  // for(uint32_t i = 0; i < nUsers + 1; i++) {
  //   Ptr<Node> deviceNode = nodes.Get(i);
  //   Ptr<Ipv4> ipv4 = deviceNode->GetObject<Ipv4> ();
  //   NS_ASSERT_MSG(ipv4, "Ipv4AddressHelper::Assign(): NetDevice is associated"
  //                 " with a node without IPv4 stack installed -> fail "
  //                 "(maybe need to use InternetStackHelper?)");
  //   uint32_t nNetDevices = deviceNode->GetNDevices();
  //   Ipv4Address address = ipv4Helper.NewAddress();
  //   for(uint32_t j = 0; j < nNetDevices; j++) {
  //     Ptr<NetDevice> netDevice = deviceNode->GetDevice(j);
  //     int32_t interface = ipv4->GetInterfaceForDevice(netDevice);
  //     if (interface == -1)
  //     {
  //       interface = ipv4->AddInterface(netDevice);
  //     }
  //     NS_ASSERT_MSG(interface >= 0, "Ipv4AddressHelper::Assign(): "
  //                   "Interface index not found");
  //     Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress(address, Ipv4Mask("255.255.255.0"));
  //     ipv4->AddAddress (interface, ipv4Addr);
  //     ipv4->SetMetric (interface, 1);
  //     ipv4->SetUp (interface);
  //   }
  //   ipv4Addresses.push_back(address);
  // }

  // PTP Application
  // Store static PTP nodes
  std::vector<PtpNode *> staticNodes(nUsers);
  // Socket to neighbor node
  std::vector<std::vector<Ptr<Socket>>> neighbor(nUsers);
  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

  uint16_t srcPort, dstPort;
  int socket_count = 0;
  
  // List of socket links established in the network
  std::vector<SocketLink *> socketLinks(2 * (nUsers - 1));

  PTPNetwork ptpTest(nUsers, packetSize, interPacketInterval, logdir);
  staticNodes[0] = new PtpNode(0, 0, 0, deviceIpv4InterfaceContainer.GetAddress(0));
  ptpTest.addNode(staticNodes[0]);

  // socketIndex[0] = -1;
  for(uint32_t i = 1; i < nUsers; i++) {
    staticNodes[i] = new PtpNode(i, 0, i, deviceIpv4InterfaceContainer.GetAddress(i));
    // All nodes are neighbor of global master clock
    // Create an UDP Socket on node i and add it to neighbor socket vectors.
    neighbor[0].push_back(Socket::CreateSocket(nodes.Get(0), tid));
    // The port is (i + 1) * 100 + j
    srcPort = 100 + i;
    // The destination port is (i + 1) * 100
    dstPort = 100 * (i + 1);
    // Bind to source IP and source port
    neighbor[0][i-1]->Bind(InetSocketAddress(deviceIpv4InterfaceContainer.GetAddress(0), srcPort));
    // Connect the socket to neighbor identified in neighborList[i][j]
    neighbor[0][i-1]->Connect(InetSocketAddress(
      deviceIpv4InterfaceContainer.GetAddress(i), dstPort
    ));
    // Set Receive Callback for each socket
    neighbor[0][i-1]->SetRecvCallback(
      MakeCallback(&PTPNetwork::receivePacket, &ptpTest)
    );
    // Create socket Link
    socketLinks[socket_count] = new SocketLink(
      0, i, 
      deviceIpv4InterfaceContainer.GetAddress(0), srcPort,
      deviceIpv4InterfaceContainer.GetAddress(i), dstPort,
      neighbor[0][i-1]
    );
    // Add Neighbor
    staticNodes[0]->addNeighbor(
      i, socketLinks[socket_count]
    );
    // Add to PTP Network
    ptpTest.addSocketLink(socketLinks[socket_count]);
    socket_count ++;
    // Create the return UDP socket on node i and add it to neighbor socket vectors.
    neighbor[i].push_back(Socket::CreateSocket(nodes.Get(i), tid));
    // The port is (i + 1) * 100 + j
    srcPort = 100 + i;
    // The destination port is (i + 1) * 100
    dstPort = 100 * (i + 1);
    // Bind to source IP and source port
    neighbor[i][0]->Bind(InetSocketAddress(deviceIpv4InterfaceContainer.GetAddress(i), dstPort));
    // Connect the socket to neighbor identified in neighborList[i][j]
    neighbor[i][0]->Connect(InetSocketAddress(
      deviceIpv4InterfaceContainer.GetAddress(0), srcPort
    ));
    // Set Receive Callback for each socket
    neighbor[i][0]->SetRecvCallback(
      MakeCallback(&PTPNetwork::receivePacket, &ptpTest)
    );
    // Create socket Link
    socketLinks[socket_count] = new SocketLink(
      i, 0, 
      deviceIpv4InterfaceContainer.GetAddress(i), dstPort,
      deviceIpv4InterfaceContainer.GetAddress(0), srcPort,
      neighbor[i][0]
    );
    // Add Neighbor
    staticNodes[i]->addNeighbor(
      0, socketLinks[socket_count]
    );
    // Add to PTP Network
    ptpTest.addSocketLink(socketLinks[socket_count]);
    socket_count++;
    // Print information to console
    std::cout << "[PTP Socket]: Master Node " << 0 << " " << 
      deviceIpv4InterfaceContainer.GetAddress(0) << ":" << srcPort 
      << " <--> " << 
      "Slave Node " << i << " " <<
      deviceIpv4InterfaceContainer.GetAddress(i) << ":" << dstPort << 
      std::endl;
    ptpTest.addNode(staticNodes[i]);
  }

  // On-off application
  DataRate linkBandwidth = DataRateValue(bandwidth).Get();
  DataRate trafficDataRate = DataRate(linkBandwidth.GetBitRate() * utilization);

  NS_LOG_INFO("Create Application.");
  OnOffHelper onoff(
    "ns3::TcpSocketFactory", 
    Address(InetSocketAddress(
      deviceIpv4InterfaceContainer.GetAddress(nUsers), 8000
    ))
  );
  onoff.SetConstantRate(trafficDataRate);

  // Install sender on all nodes, starting at 1.0s and ends at 10.0s
  ApplicationContainer app;
  for(uint32_t i = 0; i < nUsers; i++) {
    app = onoff.Install(nodes.Get(i));
    app.Start(Seconds(1.0));
    app.Stop(Seconds(1001.0));
  }

  // Install sink on traffic generator
  PacketSinkHelper sink(
    "ns3::TcpSocketFactory", 
    Address (InetSocketAddress (Ipv4Address::GetAny(), 8000))
  );
  app = sink.Install(nodes.Get(nUsers));
  app.Start(Seconds(0.));
  app.Stop(Seconds(1001.0));

  // Pcap tracing
  csma.EnablePcapAll(logdir + "traffic-bridge", false);

  AnimationInterface anim(logdir + "ptp-csma.xml");
  for(uint32_t i = 0; i < nUsers; i++) {
    if(i == 0) {
      anim.SetConstantPosition(nodes.Get(i), 10.0, 10.0);
    } else {
      anim.SetConstantPosition(nodes.Get(i), 10.0 * (i + 1), 20.0);
      anim.SetConstantPosition(csmaSwitches.Get(i - 1), 10.0 * (i + 1), 10.0);
    }
  }
  anim.SetConstantPosition(nodes.Get(nUsers), 5.0 * (1 + nUsers), 5.0);
  anim.EnablePacketMetadata(true);

  int clkOffsetCounterId = anim.AddNodeCounter(
    "offset_error", AnimationInterface::DOUBLE_COUNTER
  );
  ptpTest.setAnimationInterface(
    &anim, clkOffsetCounterId
  );
  ptpTest.setSimulationIterations(1000);
  
  Simulator::ScheduleWithContext(
    neighbor[0][0]->GetNode()->GetId(),
    Seconds(1.0),
    &PTPNetwork::startPTPProtocol,
    &ptpTest
  );

  // Save traces
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  ptpTest.closeLogs();
  NS_LOG_INFO ("Done.");
}
