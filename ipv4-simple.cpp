/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-westwood.h"

namespace ns3 {

/**
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                        
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5; 
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
main(int argc, char* argv[])
{
  uint16_t numberOfNodes = 5;
  uint16_t distance = 400;
  uint16_t simTime = 10;

  std::string tcpVariant = "TcpNewReno";

  tcpVariant = std::string ("ns3::") + tcpVariant;
  // Select TCP variant
  if (tcpVariant.compare ("ns3::TcpWestwoodPlus") == 0)
    {
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));
    }
  
  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (64));

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("10p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(numberOfNodes);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Mb/s")));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer p2pinterA = p2p.Install(nodes.Get(0), nodes.Get(1));
  NetDeviceContainer p2pinterB = p2p.Install(nodes.Get(1), nodes.Get(2));
  NetDeviceContainer p2pinterC = p2p.Install(nodes.Get(2), nodes.Get(3));
  NetDeviceContainer p2pinterD = p2p.Install(nodes.Get(3), nodes.Get(4));

  // Install internet stack on all nodes
  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer inetfaceA = address.Assign (p2pinterA);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer inetfaceB = address.Assign (p2pinterB);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer inetfaceC = address.Assign (p2pinterC);

  address.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer inetfaceD = address.Assign (p2pinterD);
  Ipv4Address sinkHostAddr = inetfaceD.GetAddress (1);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install (nodes);

  // Installing applications

  uint16_t port = 9;

  ApplicationContainer sinkApp;
  ApplicationContainer remoteApp;
  // Consumer
  PacketSinkHelper consumerHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  sinkApp.Add (consumerHelper.Install(nodes.Get(4)));
  sink = StaticCast<PacketSink> (sinkApp.Get (0));

  // Producer
  BulkSendHelper producerHelper ("ns3::TcpSocketFactory", InetSocketAddress (sinkHostAddr, port));
  producerHelper.SetAttribute("MaxBytes", UintegerValue (1000000000));
  producerHelper.SetAttribute("SendSize", UintegerValue (1040));
  remoteApp.Add (producerHelper.Install(nodes.Get(0)));

  //sinkApp.Start (Seconds (0.0));
  //remoteApp.Start (Seconds (0.0));

  Ptr<Node> n1 = nodes.Get (3);
  Ptr<Ipv4> ipv4 = n1->GetObject<Ipv4> ();
  uint32_t ipv4Index = 2;

  Simulator::Schedule (Seconds(0), &CalculateThroughput);
  //Simulator::Schedule (Seconds(2.0), &Ipv4::SetDown, ipv4, ipv4Index);
  //Simulator::Schedule (Seconds(5.0), &Ipv4::SetUp, ipv4, ipv4Index);
  Simulator::Stop(Seconds(simTime + 1));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
