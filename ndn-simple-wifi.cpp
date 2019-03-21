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

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.WifiExample");

void
TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Total energy consumed by radio = " << totalEnergy << "J");
}

static void 
CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();
  std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}

int
main(int argc, char* argv[])
{
  
  std::string phyMode ("DsssRate1Mbps");
  uint16_t numberOfnodes = 30;
  uint16_t sNode = 1;
  double totalConsumption = 0.0;
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
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("10p"));

  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("2s"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", StringValue ("0|200|0|200"));

  NodeContainer nodes;
  nodes.Create (numberOfnodes);
  NodeContainer sinkNode;
  sinkNode.Create (sNode);

  NodeContainer allNodes = NodeContainer (nodes, sinkNode);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode), 
                                "ControlMode", StringValue (phyMode));

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());
  wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set ("TxGain", DoubleValue (1));
  wifiPhy.Set ("RxGain", DoubleValue (-10));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79));

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer wifiDev = wifi.Install (wifiPhy, wifiMacHelper, nodes);
  NetDeviceContainer wifiSink = wifi.Install (wifiPhy, wifiMacHelper, sinkNode);
/*
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.InstallAll ();
*/
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("2s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                             "Bounds", StringValue ("0|200|0|200"));
  mobility.InstallAll ();
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback (&CourseChange));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll ();

  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/ncc");

  BasicEnergySourceHelper basicEnergySourceHelper;
  basicEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));
  EnergySourceContainer sources = basicEnergySourceHelper.Install (nodes);
  EnergySourceContainer srcSink = basicEnergySourceHelper.Install (sinkNode);

  WifiRadioEnergyModelHelper wifiRadioEnergyModelHelper;
  wifiRadioEnergyModelHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  wifiRadioEnergyModelHelper.Set ("RxCurrentA", DoubleValue (0.0197));
  DeviceEnergyModelContainer deviceEnergy = wifiRadioEnergyModelHelper.Install (wifiDev, sources);
  DeviceEnergyModelContainer sinkEnergy = wifiRadioEnergyModelHelper.Install (wifiSink, srcSink);
/*
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/test/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("64"));  
  auto proapp = producerHelper.Install (nodes);
  //proapp.Stop (Seconds (simTime));

  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  //consumerHelper.SetAttribute("Batches", StringValue("1s 1 10s 1 20s 1 30s 1"));
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetPrefix("/test/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("1"));
  consumerHelper.SetAttribute("NumberOfContents", StringValue("1"));
  auto cunapp = consumerHelper.Install (sinkNode);
  //cunapp.Stop (Seconds (simTime));
*/
  PacketSinkHelper producerHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  producerHelper.Install (sinkNode);

  BulkSendHelper consumerHelper ("ns3::TcpSocketFactory", InetSocketAddress (inetAddr, port));
  consumerHelper.SetAttribute ("MaxBytes", UintegerValue (1000));
  consumerHelper.SetAttribute ("SendSize", UintegerValue (64));
  consumerHelper.Install (nodes);

  ndn::GlobalRoutingHelper::CalculateRoutes();
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  for (uint32_t u = 0; u < nodes.GetN (); u++)
    {
      Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (sources.Get(u));
      Ptr<DeviceEnergyModel> basicRadioModels = basicEnergySource->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get(0);
      Ptr<WifiRadioEnergyModel> ptr = DynamicCast<WifiRadioEnergyModel> (basicRadioModels);
      
      NS_ASSERT (basicRadioModels != NULL);
      //ptr->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
/*
      if (u == 0)
        {
          double producerEnergy = ptr->GetTotalEnergyConsumption ();
          NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                << "s producer energy consumed by radio = " << producerEnergy * 100 << "mJ");
          continue;
        }
*/
      double energyConsumption = ptr->GetTotalEnergyConsumption ();
      totalConsumption += ptr->GetTotalEnergyConsumption ();

      NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                << "s energy consumed by radio = " << energyConsumption * 100 << "mJ");
      if (u == 0)
        NS_LOG_UNCOND ("Total AVG energy consumed by radio = " << (totalConsumption) * 100 << "mJ");
      else
        NS_LOG_UNCOND ("Total AVG energy consumed by radio = " << (totalConsumption / u) * 100 << "mJ");
    }
  
  Ptr<BasicEnergySource> basicEnergySrcSink = DynamicCast<BasicEnergySource> (srcSink.Get (0));
  Ptr<DeviceEnergyModel> basicRadioSrcSink = basicEnergySrcSink->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get(0);
  Ptr<WifiRadioEnergyModel> wifisrcSink = DynamicCast<WifiRadioEnergyModel> (basicRadioSrcSink);
  double energyConsumptionSink = wifisrcSink->GetTotalEnergyConsumption ();
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                << "s energy consumed by radio = " << energyConsumptionSink * 100 << "mJ");

  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
