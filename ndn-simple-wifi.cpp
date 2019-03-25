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
#include "ns3/energy-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/log.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.WifiExample");

int
main(int argc, char* argv[])
{
  std::string flow_name ("n-node-ppp.xml");
  std::string anim_name ("n-node-ppp.anim.xml");
  std::string phyMode = "HtMcs7";
  uint16_t numberOfnodes = 30;
  uint16_t sNode = 1;
  double txPowerStart = 0.0;
  double txPowerEnd = 15.0;
  double simTime = 30.0;

  CommandLine cmd;
  cmd.Parse(argc, argv);

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

  //NodeContainer cm1 = NodeContainer (nodes.Get (0), nodes.Get (1), nodes.Get (2), nodes.Get (3), nodes.Get (4));

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
  wifiPhy.Set ("TxPowerLevels", UintegerValue (16));
  wifiPhy.Set ("TxGain", DoubleValue (1));
  wifiPhy.Set ("RxGain", DoubleValue (-10));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");

  WifiMacHelper wifiMacHelper;
  Ssid ssid1 = Ssid ("ssid1");

  wifiMacHelper.SetType("ns3::AdhocWifiMac");
  //wifiMacHelper.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid1));
  //NetDeviceContainer wifiAPch1 = wifi.Install (wifiPhy, wifiMacHelper, sinkNode.Get (0));

  //wifiMacHelper.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid1));
  //NetDeviceContainer wifiSTAch1 = wifi.Install (wifiPhy, wifiMacHelper, nodes);

  NetDeviceContainer wifiDev = wifi.Install (wifiPhy, wifiMacHelper, allNodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=60]"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("10s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|200|0|200"));
  mobility.Install (allNodes);

  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  //ndnHelper.setCsSize(2); // allow just 2 entries to be cached
  //ndnHelper.setPolicy("nfd::cs::lru");
  //ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install (allNodes);

  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/self-learning");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/ncc");
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/test");
  producerHelper.SetAttribute("PayloadSize", StringValue("1000"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(1.0))); 
  producerHelper.SetAttribute("Signature", UintegerValue(100));
  producerHelper.SetAttribute("KeyLocator", StringValue("/unique/key/locator")); 
  ApplicationContainer proapp = producerHelper.Install (nodes);
  //proapp.Start (Seconds (0.0));
  //proapp.Stop (Seconds (10.0));

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  //consumerHelper.SetAttribute("Batches", StringValue("1s 1 10s 1 20s 1 30s 1"));
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetPrefix("/test/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  //consumerHelper.SetAttribute("NumberOfContents", StringValue("1"));
  ApplicationContainer cunapp = consumerHelper.Install (sinkNode.Get (0));
  //cunapp.Start (Seconds (1.0));
  //cunapp.Stop (Seconds (10.0));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
/*
  AnimationInterface anim (anim_name.c_str ());
  anim.EnablePacketMetadata ();
  anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10));
*/
  ndn::GlobalRoutingHelper::CalculateRoutes();
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  monitor->CheckForLostPackets ();
  flowmon.SerializeToXmlFile (flow_name.c_str(), true, true);

  ndn::L3RateTracer::InstallAll("wifi-rate-trace.txt", Seconds(0.5));

  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
