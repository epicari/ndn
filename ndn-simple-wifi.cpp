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
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/csma-helper.h"
#include "ns3/bridge-helper.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.WifiExample");

int
main(int argc, char* argv[])
{
  std::string phyMode("OfdmRate24Mbps");
  uint16_t numberOfnodes = 20;
  double simTime = 20.0;

  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

  CommandLine cmd;
  cmd.Parse(argc, argv);
  
  // Nodes
  NodeContainer nodes;
  nodes.Create (numberOfnodes);

  //NodeContainer apNodes;
  //apNodes.Create (numberOfnodes);

  NodeContainer router;
  router.Create (numberOfnodes);
  
  NodeContainer producer;
  producer.Create (numberOfnodes);

  // wifi Ad-hoc
  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode), 
                                "ControlMode", StringValue (phyMode));

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel", "Rss", DoubleValue (-77));

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());
/*
  wifiPhy.Set ("TxPowerStart", DoubleValue (txPowerStart));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txPowerEnd));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (14));
  wifiPhy.Set ("TxGain", DoubleValue (10));
  wifiPhy.Set ("RxGain", DoubleValue (7));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
*/
  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer wifiDev = wifi.Install (wifiPhy, wifiMacHelper, nodes);

  // Router
  for (uint32_t i = 0; i < numberOfnodes; ++i)
    {
      NodeContainer csmaDevs = NodeContainer (nodes.Get (i), router);

      CsmaHelper csma;
      csma.SetChannelAttribute ("DataRate",
                              DataRateValue (DataRate (5000000)));
      csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
      NetDeviceContainer csmaDevices = csma.Install (csmaDevs);
    }

  // Wifi AP-STA
  for (uint32_t i = 0; i < numberOfnodes; ++i)
    {
      NodeContainer infra = NodeContainer (nodes, producer);

      WifiHelper wifiInfra;
      wifiInfra.SetRemoteStationManager ("ns3::ArfWifiManager");
      
      Ssid ssid = Ssid ("wifi-default");
      
      WifiMacHelper wifiMacInfra;
      wifiMacInfra.SetType("ns3::StaWifiMac",
                            "Ssid", SsidValue (ssid));
      NetDeviceContainer staDev = wifi.Install (wifiPhy, wifiMacInfra, producer);

      wifiMacInfra.SetType("ns3::ApWifiMac",
                            "Ssid", SsidValue (ssid),
                            "BeaconInterval", TimeValue (Seconds (2.5)));
      NetDeviceContainer apDev = wifi.Install (wifiPhy, wifiMacInfra, nodes.Get (i));  
    }

  // ndn Stack
  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru", "MaxSize", "1000");
  //ndnHelper.SetDefaultRoutes (true);
  //ndnHelper.Install (nodes);
  ndnHelper.InstallAll ();

  // Mobility
  MobilityHelper mobility;
  
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("400.0"),
                                 "Y", StringValue ("400.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                             "Bounds", StringValue ("0|800|0|800"));
  
  mobility.InstallAll ();

  //Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  //positionAlloc->Add (Vector (0, 200, 0));
  //positionAlloc->Add (Vector (200, 0, 0));
  //positionAlloc->Add (Vector (200, 400, 0));
  //positionAlloc->Add (Vector (400, 200, 0));
  //positionAlloc->Add (Vector (200, 200, 0));
  //mobility.SetPositionAllocator (positionAlloc);
  //mobility.Install (producer);
  //mobility.Install (apNodes);

  string prefix = "/ucla/hello";

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  //ndnGlobalRoutingHelper.Install (nodes);
  ndnGlobalRoutingHelper.InstallAll ();
  ndnGlobalRoutingHelper.AddOrigins(prefix, producer);

  //ndn::StrategyChoiceHelper::InstallAll(prefix, "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll(prefix, "/localhost/nfd/strategy/broadcast");
  //ndn::StrategyChoiceHelper::InstallAll(prefix, "/localhost/nfd/strategy/best-route");
  
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  //producerHelper.SetAttribute("Freshness", TimeValue(Seconds(30.0))); 
  producerHelper.Install (producer);

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  consumerHelper.Install (nodes);

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds (1.0));
  ndn::CsTracer::InstallAll("cs-trace.txt", Seconds (1.0));
  ndn::AppDelayTracer::InstallAll("app-delay-tracer.txt");

  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
