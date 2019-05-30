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
#include "ns3/point-to-point-module.h"
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
  std::string phyMode = "HtMcs7";
  //uint16_t disSink = 100;
  //uint16_t disNode = 35;
  uint16_t numberOfnodes = 50;
  //uint16_t sNode = 2;
  double txPowerStart = 0.0;
  double txPowerEnd = 10.0;
  double simTime = 60.0;

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

  NodeContainer nodesA;
  nodesA.Create (numberOfnodes);
/*
  NodeContainer nodesB;
  nodesB.Create (1);

  NodeContainer nodes = NodeContainer (nodesA, nodesB);
  
  NodeContainer sinkNode;
  sinkNode.Create (sNode);

  NodeContainer allNodes = NodeContainer (sinkNode, nodes);
*/
  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll ();
  //ndnHelper.Install (sNode);
  //ndnHelper.Install (nodes);

  //CsmaHelper csma;
  //NetDeviceContainer backboneDevices = csma.Install (sinkNode);

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

  Ssid ssid = Ssid ("wifi-default");

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer wifiDev = wifi.Install (wifiPhy, wifiMacHelper, allNodes);
/*
  wifiMacHelper.SetType("ns3::StaWifiMac",
                         "ActiveProbing", BooleanValue (true),
                         "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevs = wifi.Install(wifiPhy, wifiMacHelper, nodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  for(uint16_t i = 0; i < sNode; i++)
    {
      wifiMacHelper.SetType("ns3::ApWifiMac",
                            "Ssid", SsidValue (ssid));
      NetDeviceContainer apDevs = wifi.Install(wifiPhy, wifiMacHelper, sinkNode.Get (i));

      BridgeHelper bridge;
      NetDeviceContainer bridgeDev = bridge.Install (sinkNode.Get (i), NetDeviceContainer (apDevs, backboneDevices.Get (i)));
          
      if(i==0) {
        positionAlloc->Add (Vector(100, 100, 0));
        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(sinkNode.Get (i));
      }

      positionAlloc->Add (Vector(120, 120, 0));
      mobility.SetPositionAllocator(positionAlloc);
      mobility.Install(sinkNode.Get (i));
    }
*/
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                 "Mode", StringValue ("Time"),
                                 "Time", StringValue ("2s"),
                                 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                 "Bounds", StringValue ("0|200|0|200"));

  mobility.Install(nodesA);

  //positionAlloc->Add (Vector(120, 90, 0));
  //mobility.SetPositionAllocator(positionAlloc);
  //mobility.Install(nodesB);

/*
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  for (uint16_t i = 0; i < numberOfnodes; i++) {
    positionAlloc->Add (Vector(disNode * i, 0, 2));
  }

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc); 
  mobility.Install (nodes);

  for (uint16_t j = 0; j < sNode; j++) {
    if(j == 0){
      positionAlloc->Add (Vector(disSink, 0, 6));
    }
    else
      positionAlloc->Add (Vector(disSink * j, 0, 6));    
  }
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (sinkNode);
*/

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.Install (nodesA);

  string prefix = "/ucla/hello";

  //ndn::StrategyChoiceHelper::InstallAll(prefix, "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll(prefix, "/localhost/nfd/strategy/best-route");
  
  ndnGlobalRoutingHelper.AddOrigins(prefix, nodesA);

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(5.0))); 
  producerHelper.Install (nodesA.Get (49));

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", StringValue("1"));
  consumerHelper.Install (nodesA.Get (0));

  ndn::GlobalRoutingHelper::CalculateRoutes();
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

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
