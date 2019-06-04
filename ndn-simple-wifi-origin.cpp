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
#include "ns3/bridge-helper.h"

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
  std::string phyMode = "HtMcs7";
  // disable fragmentation
  //Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
  //Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
  //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  //////////////////////
  //////////////////////
  //////////////////////
  WifiHelper wifi;
  // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  //wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
  //wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
  //                             StringValue("OfdmRate24Mbps"));
  wifi.SetStandard(WIFI_PHY_STANDARD_80211n_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode), 
                                "ControlMode", StringValue ("HtMcs0"));

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (-77));

  // YansWifiPhy wifiPhy = YansWifiPhy::Default();
  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
  wifiPhyHelper.SetChannel(wifiChannel.Create());
  //wifiPhyHelper.Set ("TxPowerStart", DoubleValue(5));
  //wifiPhyHelper.Set ("TxPowerEnd", DoubleValue(5));

  WifiMacHelper wifiMacHelper;
  //wifiMacHelper.SetType("ns3::AdhocWifiMac");

  Ssid ssid = Ssid ("wifi-default");
/*
  Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable>();
  randomizer->SetAttribute("Min", DoubleValue(10));
  randomizer->SetAttribute("Max", DoubleValue(100));
*/
  MobilityHelper mobility;
//  mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizer),
//                                "Y", PointerValue(randomizer), "Z", PointerValue(randomizer));

//  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("400.0"),
                                 "Y", StringValue ("0.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
/*
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                 "Bounds", StringValue ("0|200|0|200"));
*/
  NodeContainer nodes;
  nodes.Create (30);

  NodeContainer apNode;
  apNode.Create (1);

  NodeContainer remoteHost;
  remoteHost.Create (1);

  //NodeContainer allNodes = NodeContainer (nodes, apNode, remoteHost);

  //CsmaHelper csma;
  //NetDeviceContainer csmaDevs = csma.Install (apNode);

  ////////////////
  // 1. Install Wifi
  //NetDeviceContainer wifiNetDevices = wifi.Install(wifiPhyHelper, wifiMacHelper, allNodes);

  wifiMacHelper.SetType("ns3::StaWifiMac",
                         "Ssid", SsidValue (ssid));

  NetDeviceContainer staDevs = wifi.Install(wifiPhyHelper, wifiMacHelper, nodes);
  NetDeviceContainer remoteDevs = wifi.Install(wifiPhyHelper, wifiMacHelper, remoteHost);

//for (uint16_t i = 0; i < 2; i++)
  //{
      wifiMacHelper.SetType("ns3::ApWifiMac",
                            "Ssid", SsidValue (ssid));
      NetDeviceContainer apDevs = wifi.Install(wifiPhyHelper, wifiMacHelper, apNode);

      //BridgeHelper bridge;
      //NetDeviceContainer bridgeDev = bridge.Install (apNode.Get (i), NetDeviceContainer (apDevs, csmaDevs.Get (1)));
  //}  

  // 2. Install Mobility model
  mobility.Install (nodes);

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (200, 6, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (apNode);

  positionAlloc->Add (Vector (200, 10, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (remoteHost);
/*
  positionAlloc->Add (Vector (700, 6, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (apNode.Get (1));

  positionAlloc->Add (Vector (700, 0, 0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (remoteHost);
*/
  // 3. Install NDN stack
  NS_LOG_INFO("Installing NDN stack");
  ndn::StackHelper ndnHelper;
  // ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback
  // (MyNetDeviceFaceCallback));
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  //ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll ();

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  string prefix = "/ucla/hello";

  ndnGlobalRoutingHelper.AddOrigins(prefix, nodes);
  ndnGlobalRoutingHelper.AddOrigins(prefix, apNode);
  ndnGlobalRoutingHelper.AddOrigins(prefix, remoteHost);

  // Set BestRoute strategy
  //ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll (prefix, "/localhost/nfd/strategy/multicast");

  // 4. Set up applications
  NS_LOG_INFO("Installing Applications");

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", DoubleValue(5.0));
  auto counapp = consumerHelper.Install (nodes);
  counapp.Start (Seconds (0.0));
  counapp.Stop (Seconds (30.0));

  counapp.Start (Seconds (31.0));
  counapp.Stop (Seconds (61.0));
/*
  auto counappA = consumerHelper.Install(remoteHost.Get (0));
  auto counappB = consumerHelper.Install(remoteHost.Get (1));

  counappA.Start (Seconds (0.0));
  counappA.Stop (Seconds (30.0));

  counappB.Start (Seconds (31.0));
  counappB.Stop (Seconds (61.0));
*/

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1200"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(60.0))); 
  //producerHelper.Install(nodes);

  auto prodappA = producerHelper.Install (remoteHost);

  //prodappA.Start (Seconds (0.0));
  //prodappA.Stop (Seconds (30.0));

  ////////////////
  ndn::GlobalRoutingHelper::CalculateRoutes();
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
