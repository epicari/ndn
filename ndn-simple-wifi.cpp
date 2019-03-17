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

void
RemainingEnergy (double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Current remaining energy = " << remainingEnergy << "J");
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
  
  std::string phyMode ("DsssRate1Mbps");
  uint16_t numberOfnodes = 10;
  //double IdleCurrent = 0.0;
  //double TxCurrent = 0.0;
  //double RxCurrent = 0.0;

  CommandLine cmd;
  cmd.Parse(argc, argv);
  
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));
/*
  NodeContainer n0, n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11;
  n0.Create (1);
  n1.Create (1);
  n2.Create (1);
  n3.Create (1);
  n4.Create (1);
  n5.Create (1);
  n6.Create (1);
  n7.Create (1);
  n8.Create (1);
  n9.Create (1);
  n10.Create (1);
  n11.Create (1);

  NodeContainer nodecm1 = NodeContainer (n0, n1, n2);
  NodeContainer nodecm2 = NodeContainer (n3, n4, n5);
  NodeContainer nodecm3 = NodeContainer (n6, n7, n8);
  NodeContainer nodecm4 = NodeContainer (n9, n10, n11);
  NodeContainer nodech1 = NodeContainer (nodecm1, nodecm2);
  NodeContainer nodech2 = NodeContainer (nodecm3, nodecm4);
  NodeContainer Allnodes = NodeContainer (nodech1, nodech2);
*/

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

  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
  wifiPhyHelper.SetChannel(wifiChannel.Create());

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer wifiDev = wifi.Install (wifiPhyHelper, wifiMacHelper, nodes);

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

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/test/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("10"));

  auto proapp = producerHelper.Install (nodes.Get (0));

  auto cunappn0 = consumerHelper.Install (nodes.Get (1));
  cunappn0.Start (Seconds (1.0));
  cunappn0.Stop (Seconds (2.0));

  auto cunappn1 = consumerHelper.Install (nodes.Get (2));
  cunappn1.Start (Seconds (2.1));
  cunappn1.Stop (Seconds (3.0));

  auto cunappn2 = consumerHelper.Install (nodes.Get (3));
  cunappn2.Start (Seconds (3.1));
  cunappn2.Stop (Seconds (4.0));

  auto cunappn3 = consumerHelper.Install (nodes.Get (4));
  cunappn3.Start (Seconds (4.1));
  cunappn3.Stop (Seconds (5.0));

  auto cunappn4 = consumerHelper.Install (nodes.Get (5));
  cunappn4.Start (Seconds (5.1));
  cunappn4.Stop (Seconds (6.0));

  auto cunappn5 = consumerHelper.Install (nodes.Get (6));
  cunappn5.Start (Seconds (6.1));
  cunappn5.Stop (Seconds (7.0));

  auto cunappn6 = consumerHelper.Install (nodes.Get (7));
  cunappn6.Start (Seconds (7.1));
  cunappn6.Stop (Seconds (8.0));

  auto cunappn7 = consumerHelper.Install (nodes.Get (8));
  cunappn7.Start (Seconds (8.1));
  cunappn7.Stop (Seconds (9.0));

  auto cunappn8 = consumerHelper.Install (nodes.Get (9));
  cunappn8.Start (Seconds (9.1));
  cunappn8.Stop (Seconds (10.0));

  auto cunappn9 = consumerHelper.Install (nodes.Get (10));
  cunappn9.Start (Seconds (10.1));
  cunappn9.Stop (Seconds (11.0));

  Simulator::Stop(Seconds(30.0));
  Simulator::Run();

  //for (EnergySourceContainer::Iterator sourceIter = sources.Begin (); sourceIter != sources.End (); sourceIter ++)
  for (uint16_t i = 0; i <= numberOfnodes; i++)
    {
      Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (nodes.Get(i));
      basicEnergySource->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
      Ptr<DeviceEnergyModel> basicRadioModels = basicEnergySource->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get(0);
      Ptr<WifiRadioEnergyModel> ptr = DynamicCast<WifiRadioEnergyModel> (basicRadioModels);
      NS_ASSERT (basicRadioModels != NULL);
      basicRadioModels->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
    }
  for (DeviceEnergyModelContainer::Iterator iter = deviceEnergy.Begin (); iter != deviceEnergy.End (); iter ++)
    {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                      << "s) Total energy consumed by radio = " << energyConsumed << "J");
      NS_ASSERT (energyConsumed <= 0.1);
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
