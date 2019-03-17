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

int
main(int argc, char* argv[])
{
  
  std::string phyMode ("DsssRate1Mbps");
  uint16_t numberOfnodes = 26;
  double totalConsumption = 0.0;
  double simTime = 270.0;

  CommandLine cmd;
  cmd.Parse(argc, argv);
  
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  NodeContainer nodes;
  nodes.Create (numberOfnodes);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode), 
                                "ControlMode", StringValue (phyMode));

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");
  wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());
  wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set ("TxGain", DoubleValue (0));
  wifiPhy.Set ("RxGain", DoubleValue (0));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer wifiDev = wifi.Install (wifiPhy, wifiMacHelper, nodes);

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfnodes; i++)
    {
      positionAlloc->Add (Vector(2 * i, 2 * i, 0));
    }

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(nodes);

  NS_LOG_INFO("Installing NDN stack");
  ndn::StackHelper ndnHelper;
  // ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback
  // (MyNetDeviceFaceCallback));
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(nodes);

  //ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/best-route");

  BasicEnergySourceHelper basicEnergySourceHelper;
  basicEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));
  EnergySourceContainer sources = basicEnergySourceHelper.Install (nodes);

  WifiRadioEnergyModelHelper wifiRadioEnergyModelHelper;
  DeviceEnergyModelContainer deviceEnergy = wifiRadioEnergyModelHelper.Install (wifiDev, sources);

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/");
  producerHelper.SetAttribute("PayloadSize", StringValue("64"));
  producerHelper.Install (nodes.Get (0));

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/test/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("10"));


  auto cunappn0 = consumerHelper.Install (nodes.Get (1));
  cunappn0.Stop (Seconds (10.0));

  auto cunappn1 = consumerHelper.Install (nodes.Get (2));
  cunappn1.Start (Seconds (10.1));
  cunappn1.Stop (Seconds (20.1));

  auto cunappn2 = consumerHelper.Install (nodes.Get (3));
  cunappn2.Start (Seconds (20.2));
  cunappn2.Stop (Seconds (30.2));

  auto cunappn3 = consumerHelper.Install (nodes.Get (4));
  cunappn3.Start (Seconds (30.3));
  cunappn3.Stop (Seconds (40.3));

  auto cunappn4 = consumerHelper.Install (nodes.Get (5));
  cunappn4.Start (Seconds (40.4));
  cunappn4.Stop (Seconds (50.4));

  auto cunappn5 = consumerHelper.Install (nodes.Get (6));
  cunappn5.Start (Seconds (50.5));
  cunappn5.Stop (Seconds (60.5));

  auto cunappn6 = consumerHelper.Install (nodes.Get (7));
  cunappn6.Start (Seconds (60.6));
  cunappn6.Stop (Seconds (70.6));

  auto cunappn7 = consumerHelper.Install (nodes.Get (8));
  cunappn7.Start (Seconds (70.7));
  cunappn7.Stop (Seconds (80.7));

  auto cunappn8 = consumerHelper.Install (nodes.Get (9));
  cunappn8.Start (Seconds (80.8));
  cunappn8.Stop (Seconds (90.8));

  auto cunappn9 = consumerHelper.Install (nodes.Get (10));
  cunappn9.Start (Seconds (90.9));
  cunappn9.Stop (Seconds (100.9));

  auto cunappn10 = consumerHelper.Install (nodes.Get (11));
  cunappn10.Start (Seconds (101.0));
  cunappn10.Stop (Seconds (111.0));

  auto cunappn11 = consumerHelper.Install (nodes.Get (12));
  cunappn11.Start (Seconds (111.1));
  cunappn11.Stop (Seconds (121.1));

  auto cunappn12 = consumerHelper.Install (nodes.Get (13));
  cunappn12.Start (Seconds (121.2));
  cunappn12.Stop (Seconds (131.2));

  auto cunappn13 = consumerHelper.Install (nodes.Get (14));
  cunappn13.Start (Seconds (131.3));
  cunappn13.Stop (Seconds (141.3));

  auto cunappn14 = consumerHelper.Install (nodes.Get (15));
  cunappn14.Start (Seconds (141.4));
  cunappn14.Stop (Seconds (151.4));

  auto cunappn15 = consumerHelper.Install (nodes.Get (16));
  cunappn15.Start (Seconds (151.5));
  cunappn15.Stop (Seconds (161.5));

  auto cunappn16 = consumerHelper.Install (nodes.Get (17));
  cunappn16.Start (Seconds (161.6));
  cunappn16.Stop (Seconds (171.6));

  auto cunappn17 = consumerHelper.Install (nodes.Get (18));
  cunappn17.Start (Seconds (171.7));
  cunappn17.Stop (Seconds (181.7));

  auto cunappn18 = consumerHelper.Install (nodes.Get (19));
  cunappn18.Start (Seconds (181.8));
  cunappn18.Stop (Seconds (191.8));

  auto cunappn19 = consumerHelper.Install (nodes.Get (20));
  cunappn19.Start (Seconds (191.9));
  cunappn19.Stop (Seconds (201.9));

  auto cunappn20 = consumerHelper.Install (nodes.Get (21));
  cunappn20.Start (Seconds (202.0));
  cunappn20.Stop (Seconds (212.0));

  auto cunappn21 = consumerHelper.Install (nodes.Get (22));
  cunappn21.Start (Seconds (212.1));
  cunappn21.Stop (Seconds (222.2));

  auto cunappn22 = consumerHelper.Install (nodes.Get (23));
  cunappn22.Start (Seconds (222.3));
  cunappn22.Stop (Seconds (242.3));

  auto cunappn23 = consumerHelper.Install (nodes.Get (24));
  cunappn23.Start (Seconds (242.4));
  cunappn23.Stop (Seconds (252.4));

  auto cunappn24 = consumerHelper.Install (nodes.Get (25));
  cunappn24.Start (Seconds (252.5));
  cunappn24.Stop (Seconds (262.5));

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  
  for (uint16_t u = 0; u <= numberOfnodes; u++)
    {
      Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (sources.Get(u));
      Ptr<DeviceEnergyModel> basicRadioModels = basicEnergySource->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get(0);
      Ptr<WifiRadioEnergyModel> ptr = DynamicCast<WifiRadioEnergyModel> (basicRadioModels);
      NS_ASSERT (basicRadioModels != NULL);
      totalConsumption += ptr->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("AVG Energy Consumption: "<< totalConsumption/u);
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
