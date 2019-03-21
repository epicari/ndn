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
#include "ns3/point-to-point-module.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.WifiExample");

static bool g_verbose = true;

void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, WifiPreamble preamble)
{
  if (g_verbose)
    {
      std::cout << "PHYRXOK mode=" << mode << " snr=" << snr << " " << *packet << std::endl;
    }
}
void
PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double snr)
{
  if (g_verbose)
    {
      std::cout << "PHYRXERROR snr=" << snr << " " << *packet << std::endl;
    }
}
void
PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{
  if (g_verbose)
    {
      std::cout << "PHYTX mode=" << mode << " " << *packet << std::endl;
    }
}
void
TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Total energy consumed by radio = " << totalEnergy << "J");
}

int
main(int argc, char* argv[])
{
  
  std::string phyMode = "HtMcs7";
  uint16_t numberOfnodes = 10;
  uint16_t sNode = 2;
  uint16_t remoteNode = 1;
  double totalConsumption = 0.0;
  double simTime = 15.0;

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

  NodeContainer remoteSink;
  remoteSink.Create (remoteNode);

  NodeContainer cm1 = NodeContainer (nodes.Get (0), nodes.Get (1), nodes.Get (2), nodes.Get (3), nodes.Get (4));
  NodeContainer cm2 = NodeContainer (nodes.Get (5), nodes.Get (6), nodes.Get (7), nodes.Get (8), nodes.Get (9));

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
  wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set ("TxGain", DoubleValue (1));
  wifiPhy.Set ("RxGain", DoubleValue (-10));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");

  WifiMacHelper wifiMacHelper;
  Ssid ssid1 = Ssid ("ssid1");
  Ssid ssid2 = Ssid ("ssid2");
  Ssid ssid3 = Ssid ("ssid3");
  //wifiMacHelper.SetType("ns3::AdhocWifiMac");
  wifiMacHelper.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid1));
  NetDeviceContainer wifiAPch1 = wifi.Install (wifiPhy, wifiMacHelper, sinkNode.Get (0));

  wifiMacHelper.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid1));
  NetDeviceContainer wifiSTAch1 = wifi.Install (wifiPhy, wifiMacHelper, cm1);

  wifiMacHelper.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid2));
  NetDeviceContainer wifiAPch2 = wifi.Install (wifiPhy, wifiMacHelper, sinkNode.Get (1));

  wifiMacHelper.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid2));
  NetDeviceContainer wifiSTAch2 = wifi.Install (wifiPhy, wifiMacHelper, cm2);

  PointToPointHelper p2p;
  p2p.Install (sinkNode.Get (0), remoteSink);
  p2p.Install (remoteSink, sinkNode.Get (1));

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

  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll ();

  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/self-learning");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/ncc");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  BasicEnergySourceHelper basicEnergySourceHelper;
  basicEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));
  EnergySourceContainer sources = basicEnergySourceHelper.Install (nodes);
  EnergySourceContainer srcSink = basicEnergySourceHelper.Install (sinkNode);
/*
  WifiRadioEnergyModelHelper wifiRadioEnergyModelHelper;
  wifiRadioEnergyModelHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  wifiRadioEnergyModelHelper.Set ("RxCurrentA", DoubleValue (0.0197));
  DeviceEnergyModelContainer deviceEnergy = wifiRadioEnergyModelHelper.Install (wifiSTA, sources);
  DeviceEnergyModelContainer sinkEnergy = wifiRadioEnergyModelHelper.Install (wifiAP, srcSink);
*/
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/test/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("64"));  
  ApplicationContainer proapp = producerHelper.Install (cm1);
  ApplicationContainer proapp1 = producerHelper.Install (cm2);
  //proapp.Start (Seconds (0.0));
  proapp.Stop (Seconds (10.0));
  proapp1.Stop (Seconds (10.0));

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  //consumerHelper.SetAttribute("Batches", StringValue("1s 1 10s 1 20s 1 30s 1"));
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetPrefix("/test/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  //consumerHelper.SetAttribute("NumberOfContents", StringValue("1"));
  ApplicationContainer cunapp = consumerHelper.Install (sinkNode.Get (0));
  ApplicationContainer cunapp1 = consumerHelper.Install (sinkNode.Get (1));
  //cunapp.Start (Seconds (1.0));
  cunapp.Stop (Seconds (10.0));
  cunapp.Stop (Seconds (10.0));

  ApplicationContainer proch1 = producerHelper.Install (sinkNode.Get (0));
  ApplicationContainer proch2 = producerHelper.Install (sinkNode.Get (1));
  ApplicationContainer remoteApp = consumerHelper.Install (remoteSink);
  proch1.Start (Seconds (10.1));
  proch2.Start (Seconds (10.1));
  remoteApp.Start (Seconds (10.1));

  ndn::GlobalRoutingHelper::CalculateRoutes();
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  for (uint32_t u = 0; u < nodes.GetN (); u++)
    {
      Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (sources.Get(u));
      Ptr<DeviceEnergyModel> basicRadioModels = basicEnergySource->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get(0);
      Ptr<WifiRadioEnergyModel> ptr = DynamicCast<WifiRadioEnergyModel> (basicRadioModels);
      NS_ASSERT (basicRadioModels != NULL);
      double energyConsumption = ptr->GetTotalEnergyConsumption ();
      totalConsumption += ptr->GetTotalEnergyConsumption ();
      uint16_t n = u+1;

      NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                << "s energy consumed by radio = " << energyConsumption * 100 << "mJ");
      NS_LOG_UNCOND ("Total AVG energy consumed by radio = " << (totalConsumption / n) * 100 << "mJ");
    }
  for (uint32_t k = 0; k < sinkNode.GetN (); k++)
    {
      Ptr<BasicEnergySource> basicEnergySrcSink = DynamicCast<BasicEnergySource> (srcSink.Get (k));
      Ptr<DeviceEnergyModel> basicRadioSrcSink = basicEnergySrcSink->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get(0);
      Ptr<WifiRadioEnergyModel> wifisrcSink = DynamicCast<WifiRadioEnergyModel> (basicRadioSrcSink);
      double energyConsumptionSink = wifisrcSink->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                    << "s energy consumed by radio = " << energyConsumptionSink * 100 << "mJ");
    }
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
