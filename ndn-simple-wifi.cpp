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

int
main(int argc, char* argv[])
{
  
  std::string phyMode ("DsssRate1Mbps");
  uint16_t numberOfnodes = 26;
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

  NodeContainer nodes;
  nodes.Create (numberOfnodes);

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
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer wifiDev = wifi.Install (wifiPhy, wifiMacHelper, nodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodes);

  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(nodes);

  //ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  BasicEnergySourceHelper basicEnergySourceHelper;
  basicEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));
  EnergySourceContainer sources = basicEnergySourceHelper.Install (nodes);

  WifiRadioEnergyModelHelper wifiRadioEnergyModelHelper;
  wifiRadioEnergyModelHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  wifiRadioEnergyModelHelper.Set ("RxCurrentA", DoubleValue (0.0197));
  DeviceEnergyModelContainer deviceEnergy = wifiRadioEnergyModelHelper.Install (wifiDev, sources);


  ndn::AppHelper producerHelper("ns3::ndn::ConsumerCbr");
  ndn::AppHelper consumerHelper("ns3::ndn::Producer");

  producerHelper.SetPrefix("/");
  consumerHelper.SetAttribute("PayloadSize", StringValue("64"));

  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  
  consumerHelper.SetPrefix("/test/prefix");
  //consumerHelper.SetAttribute("Frequency", StringValue("1"));
  //consumerHelper.SetAttribute("Batches", StringValue("1s 1"));  
  //consumerHelper.Install (nodes);
  producerHelper.Install (nodes.Get (0));

  //consumerHelper.SetAttribute("Batches", StringValue("0.5s 1"));
  auto cunappn0 = consumerHelper.Install (nodes.Get (1));
  cunappn0.Stop (Seconds (1.0));

  //consumerHelper.SetAttribute("Batches", StringValue("2s 1"));
  auto cunappn1 = consumerHelper.Install (nodes.Get (2));
  cunappn1.Start (Seconds (1.1));
  cunappn1.Stop (Seconds (2.1));

  //consumerHelper.SetAttribute("Batches", StringValue("3s 1"));
  auto cunappn2 = consumerHelper.Install (nodes.Get (3));
  cunappn2.Start (Seconds (2.2));
  cunappn2.Stop (Seconds (3.2));

  //consumerHelper.SetAttribute("Batches", StringValue("4s 1"));
  auto cunappn3 = consumerHelper.Install (nodes.Get (4));
  cunappn3.Start (Seconds (3.3));
  cunappn3.Stop (Seconds (4.3));

  //consumerHelper.SetAttribute("Batches", StringValue("5s 1"));
  auto cunappn4 = consumerHelper.Install (nodes.Get (5));
  cunappn4.Start (Seconds (4.4));
  cunappn4.Stop (Seconds (5.4));

  //consumerHelper.SetAttribute("Batches", StringValue("6s 1"));
  auto cunappn5 = consumerHelper.Install (nodes.Get (6));
  cunappn5.Start (Seconds (5.5));
  cunappn5.Stop (Seconds (6.5));

  //consumerHelper.SetAttribute("Batches", StringValue("7s 1"));
  auto cunappn6 = consumerHelper.Install (nodes.Get (7));
  cunappn6.Start (Seconds (6.6));
  cunappn6.Stop (Seconds (7.6));

  //consumerHelper.SetAttribute("Batches", StringValue("8s 1"));
  auto cunappn7 = consumerHelper.Install (nodes.Get (8));
  cunappn7.Start (Seconds (7.7));
  cunappn7.Stop (Seconds (8.7));

  //consumerHelper.SetAttribute("Batches", StringValue("9s 1"));
  auto cunappn8 = consumerHelper.Install (nodes.Get (9));
  cunappn8.Start (Seconds (8.8));
  cunappn8.Stop (Seconds (9.8));

  //consumerHelper.SetAttribute("Batches", StringValue("10s 1"));
  auto cunappn9 = consumerHelper.Install (nodes.Get (10));
  cunappn9.Start (Seconds (9.9));
  cunappn9.Stop (Seconds (10.9));

  //consumerHelper.SetAttribute("Batches", StringValue("11.5s 1"));
  auto cunappn10 = consumerHelper.Install (nodes.Get (11));
  cunappn10.Start (Seconds (11.0));
  cunappn10.Stop (Seconds (12.0));

  //consumerHelper.SetAttribute("Batches", StringValue("13s 1"));
  auto cunappn11 = consumerHelper.Install (nodes.Get (12));
  cunappn11.Start (Seconds (12.1));
  cunappn11.Stop (Seconds (13.1));

  //consumerHelper.SetAttribute("Batches", StringValue("14s 1"));
  auto cunappn12 = consumerHelper.Install (nodes.Get (13));
  cunappn12.Start (Seconds (13.2));
  cunappn12.Stop (Seconds (14.2));

  //consumerHelper.SetAttribute("Batches", StringValue("15s 1"));
  auto cunappn13 = consumerHelper.Install (nodes.Get (14));
  cunappn13.Start (Seconds (14.3));
  cunappn13.Stop (Seconds (15.3));

  //consumerHelper.SetAttribute("Batches", StringValue("16s 1"));
  auto cunappn14 = consumerHelper.Install (nodes.Get (15));
  cunappn14.Start (Seconds (15.4));
  cunappn14.Stop (Seconds (16.4));

  //consumerHelper.SetAttribute("Batches", StringValue("17s 1"));
  auto cunappn15 = consumerHelper.Install (nodes.Get (16));
  cunappn15.Start (Seconds (16.5));
  cunappn15.Stop (Seconds (17.5));

  //consumerHelper.SetAttribute("Batches", StringValue("18s 1"));
  auto cunappn16 = consumerHelper.Install (nodes.Get (17));
  cunappn16.Start (Seconds (17.6));
  cunappn16.Stop (Seconds (18.6));

  //consumerHelper.SetAttribute("Batches", StringValue("19s 1"));
  auto cunappn17 = consumerHelper.Install (nodes.Get (18));
  cunappn17.Start (Seconds (18.7));
  cunappn17.Stop (Seconds (19.7));

  //consumerHelper.SetAttribute("Batches", StringValue("20s 1"));
  auto cunappn18 = consumerHelper.Install (nodes.Get (19));
  cunappn18.Start (Seconds (19.8));
  cunappn18.Stop (Seconds (20.8));

  //consumerHelper.SetAttribute("Batches", StringValue("21s 1"));
  auto cunappn19 = consumerHelper.Install (nodes.Get (20));
  cunappn19.Start (Seconds (20.9));
  cunappn19.Stop (Seconds (21.9));

  //consumerHelper.SetAttribute("Batches", StringValue("22.5s 1"));
  auto cunappn20 = consumerHelper.Install (nodes.Get (21));
  cunappn20.Start (Seconds (22.0));
  cunappn20.Stop (Seconds (23.0));

  //consumerHelper.SetAttribute("Batches", StringValue("23.5s 1"));
  auto cunappn21 = consumerHelper.Install (nodes.Get (22));
  cunappn21.Start (Seconds (23.1));
  cunappn21.Stop (Seconds (24.1));

  //consumerHelper.SetAttribute("Batches", StringValue("25s 1"));
  auto cunappn22 = consumerHelper.Install (nodes.Get (23));
  cunappn22.Start (Seconds (24.2));
  cunappn22.Stop (Seconds (25.2));

  //consumerHelper.SetAttribute("Batches", StringValue("26s 1"));
  auto cunappn23 = consumerHelper.Install (nodes.Get (24));
  cunappn23.Start (Seconds (25.3));
  cunappn23.Stop (Seconds (26.3));

  //consumerHelper.SetAttribute("Batches", StringValue("27s 1"));
  auto cunappn24 = consumerHelper.Install (nodes.Get (25));
  cunappn24.Start (Seconds (26.4));
  cunappn24.Stop (Seconds (27.4));


  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  for (uint32_t u = 0; u < nodes.GetN (); u++)
    {
      Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (sources.Get(0));
      Ptr<DeviceEnergyModel> basicRadioModels = basicEnergySource->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get(0);
      Ptr<WifiRadioEnergyModel> ptr = DynamicCast<WifiRadioEnergyModel> (basicRadioModels);
      
      NS_ASSERT (basicRadioModels != NULL);
      //ptr->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
/*
      if (u == 0)
        {
          double producerEnergy = ptr->GetTotalEnergyConsumption ();
          NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                << "s producer energy consumed by radio = " << producerEnergy << "J");
        }
*/
      double energyConsumption = ptr->GetTotalEnergyConsumption ();
      totalConsumption += ptr->GetTotalEnergyConsumption ();

      NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                << "s energy consumed by radio = " << energyConsumption << "J");
      NS_LOG_UNCOND ("Total AVG energy consumed by radio = " << totalConsumption / u << "J");
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
