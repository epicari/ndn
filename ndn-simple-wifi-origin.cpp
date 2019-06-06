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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.WifiExample");

//
// DISCLAIMER:  Note that this is an extremely simple example, containing just 2 wifi nodes
// communicating directly over AdHoc channel.
//

// Ptr<ndn::NetDeviceFace>
// MyNetDeviceFaceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn, Ptr<NetDevice> device)
// {
//   // NS_LOG_DEBUG ("Create custom network device " << node->GetId ());
//   Ptr<ndn::NetDeviceFace> face = CreateObject<ndn::MyNetDeviceFace> (node, device);
//   ndn->AddFace (face);
//   return face;
// }

int
main(int argc, char* argv[])
{
  //std::string phyMode = "HtMcs7";
  // disable fragmentation
  //Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
  //Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
  //Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  //////////////////////
  //////////////////////
  //////////////////////
  
  NodeContainer nodes;
  nodes.Create (20);

  NodeContainer apNode;
  apNode.Create (10);

  NodeContainer router;
  router.Create (2);

  NodeContainer remoteHost;
  remoteHost.Create (1);

  NodeContainer p2prouters = NodeContainer (router.Get(0), router.Get(1));
  NodeContainer p2premote = NodeContainer (router.Get(1), remoteHost);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211ac);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("VhtMcs9"), 
                                "ControlMode", StringValue ("VhtMcs0"));

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (-77));

  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
  wifiPhyHelper.SetChannel(wifiChannel.Create());
  //wifiPhyHelper.Set ("Antennas", UintegerValue (4));
  //wifiPhyHelper.Set ("MaxSupportedTxSpatialStreams", UintegerValue (2));
  //wifiPhyHelper.Set ("MaxSupportedRxSpatialStreams", UintegerValue (2));
  //wifiPhyHelper.Set ("TxPowerStart", DoubleValue(5));
  //wifiPhyHelper.Set ("TxPowerEnd", DoubleValue(5));

  Ssid ssid = Ssid ("wifi-default");

  MobilityHelper mobility;

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::StaWifiMac",
                         "Ssid", SsidValue (ssid),
                         "ActiveProbing", BooleanValue (true),
                         "ProbeRequestTimeout", TimeValue(Seconds(0.25)));

  NetDeviceContainer staDevs = wifi.Install(wifiPhyHelper, wifiMacHelper, nodes);

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                            "MinX", DoubleValue (0.0),
                            "MinY", DoubleValue (2.0),
                            "DeltaX", DoubleValue (10.0),
                            "DeltaY", DoubleValue (0.0),
                            "GridWidth", UintegerValue (20),
                            "LayoutType", StringValue ("RowFirst"));
  mobility.Install (nodes);

  wifiMacHelper.SetType("ns3::ApWifiMac",
                        "Ssid", SsidValue (ssid),
                        "BeaconGeneration", BooleanValue(false));

  NetDeviceContainer apDevs = wifi.Install(wifiPhyHelper, wifiMacHelper, apNode);

  for (uint16_t i = 0; i < apNode.GetN (); ++i)
    {
      NodeContainer csmaNodes = NodeContainer (apNode.Get (i), router.Get(0));

      CsmaHelper csma;
      csma.SetChannelAttribute ("DataRate",
                                DataRateValue (DataRate (5000000)));
      csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));
      NetDeviceContainer csmaDevs = csma.Install (csmaNodes);
    }

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                            "MinX", DoubleValue (5.0),
                            "MinY", DoubleValue (6.0),
                            "DeltaX", DoubleValue (10.0),
                            "DeltaY", DoubleValue (0.0),
                            "GridWidth", UintegerValue (10),
                            "LayoutType", StringValue ("RowFirst"));
  mobility.Install (apNode);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  p2p.Install (p2prouters);
  p2p.Install (p2premote);

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (250, 10, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (router.Get (0));

  positionAlloc->Add (Vector (750, 10, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (router.Get (1));

  positionAlloc->Add (Vector (750, 0, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (remoteHost);

  NS_LOG_INFO("Installing NDN stack");
  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  //ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll ();

  string prefix = "/ucla/hello";

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  ndnGlobalRoutingHelper.AddOrigins(prefix, remoteHost);

  ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/best-route");
  //ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/multicast");

  NS_LOG_INFO("Installing Applications");

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", DoubleValue(10.0));
  consumerHelper.Install (nodes);

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1200"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(60.0))); 
  producerHelper.Install (remoteHost);

  ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds (1.0));
  ndn::CsTracer::InstallAll("cs-trace.txt", Seconds (1.0));
  ndn::AppDelayTracer::InstallAll("app-delay-tracer.txt");

  Simulator::Stop(Seconds(61.0));

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
