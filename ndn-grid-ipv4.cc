/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2012 University of California, Los Angeles
 *
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */
// ndn-grid.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"

using namespace ns3;

/**
 * This scenario simulates a grid topology (using PointToPointGrid module)
 *
 * (consumer) -- ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) -- (producer)
 *
 * All links are 1Mbps with propagation 10ms delay. 
 *
 * FIB is populated using NdnGlobalRoutingHelper.
 *
 * Consumer requests data from producer with frequency 100 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-grid
 */

int
main (int argc, char *argv[])
{
  uint16_t numberOfNodes = 1;

  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("10"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Creating 3x3 topology
  PointToPointHelper p2p;
  PointToPointGridHelper grid (3, 3, p2p);
  grid.BoundingBox(100,100,200,200);

  // Create Node and set grid 
  NodeContainer consumer;
  consumer.Create (numberOfNodes);

  NodeContainer producer;
  producer.Create (numberOfNodes);

  NetDeviceContainer Consumerdevice = p2p.Install (consumer);
  NetDeviceContainer Producerdevice = p2p.Install (producer);

  consumer.Add (grid.GetNode (0, 0));
  producer.Add (grid.GetNode (2, 2));

  // Install Interest stack on all nodes
  InternetStackHelper internet;
  internet.InstallAll ();

  // Assign Ipv4 Address
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i;
  i = ipv4.Assign (Consumerdevice);

  ipv4.SetBase ("20.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer j;
  j = ipv4.Assign (Producerdevice);

  // Create Application
  uint16_t port = 9;
  ApplicationContainer consumerApp;
  ApplicationContainer producerApp;

  BulkSendHelper consumerHelper ("ns3::TcpSocketFactory", 
                                      InetSocketAddress (j.GetAddress (0), port));
  consumerApp.Add (consumerHelper.Install (consumer));

  PacketSinkHelper producerHelper ("ns3::TcpSocketFactory",
                                    InetSocketAddress (Ipv4Address::GetAny (), port));
  producerHelper.SetAttribute ("MaxBytes", StringValue("1024"));
  producerApp.Add (producerHelper.Install (producer));

  consumerApp.Start (Seconds (0.0));
  producerApp.Start (Seconds (0.0));

  consumerApp.Stop (Seconds (20.0));
  producerApp.Stop (Seconds (20.0));

  Simulator::Stop (Seconds (20.0));

  Simulator::Run ();
  Simulator::Destroy ();
}
