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
#include "ns3/wifi-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/mobility-module.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  std::string phyMode = "HtMcs7";
  uint16_t distance = 100;
  uint16_t numberOfnodes = 10;
  uint16_t sNode = 1;
  double txPowerStart = 0.0;
  double txPowerEnd = 10.0;
  double simTime = 10.0;

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer nodes;
  nodes.Create (numberOfnodes);
  
  NodeContainer sinkNode;
  sinkNode.Create (sNode);

  NodeContainer allNodes = NodeContainer (nodes, sinkNode);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode), 
                                "ControlMode", StringValue ("HtMcs0"));

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());
  wifiPhy.Set ("TxPowerStart", DoubleValue (txPowerStart));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txPowerEnd));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (14));
  wifiPhy.Set ("TxGain", DoubleValue (10));
  wifiPhy.Set ("RxGain", DoubleValue (7));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");

  NetDeviceContainer wifiDev = wifi.Install (wifiPhy, wifiMacHelper, allNodes);

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector(0, 0, 5));
  positionAlloc->Add (Vector(distance, 0, 2));

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (allNodes);

  // Install Interest stack on all nodes
  InternetStackHelper internet;
  internet.Install (c);

  // Assign Ipv4 Address
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i;
  i = ipv4.Assign (dcr);

  ipv4.SetBase ("20.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer j;
  j = ipv4.Assign (dpr);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Create Application
  uint16_t port = 9;
  ApplicationContainer consumerApp;
  ApplicationContainer producerApp;

  BulkSendHelper consumerHelper ("ns3::TcpSocketFactory", 
                                  Address (InetSocketAddress (j.GetAddress (0), port)));
  consumerApp.Add (consumerHelper.Install (c.Get (0)));

  PacketSinkHelper producerHelper ("ns3::TcpSocketFactory",
                                    Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  producerApp.Add (producerHelper.Install (c.Get (2)));

  consumerApp.Start (Seconds (0.0));
  producerApp.Start (Seconds (0.0));

  consumerApp.Stop (Seconds (20.0));
  producerApp.Stop (Seconds (20.0));

  Simulator::Stop (Seconds (20.0));

  Simulator::Run ();
  Simulator::Destroy ();
}
