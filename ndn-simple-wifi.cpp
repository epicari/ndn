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
  // disable fragmentation
  Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue("OfdmRate24Mbps"));

  uint16_t numberOfnodes = 10;
  double IdleCurrent = 0.0;
  double TxCurrent = 0.0;
  double RxCurrent = 0.0;

  CommandLine cmd;
  cmd.Parse(argc, argv);

  //////////////////////
  //////////////////////
  //////////////////////
  WifiHelper wifi;
  // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
                               StringValue("OfdmRate24Mbps"));

  YansWifiChannelHelper wifiChannel; // = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");
  wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel");

  // YansWifiPhy wifiPhy = YansWifiPhy::Default();
  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
  wifiPhyHelper.SetChannel(wifiChannel.Create());
  wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
  wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));

  WifiMacHelper wifiMacHelper;
  wifiMacHelper.SetType("ns3::AdhocWifiMac");

  Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable>();
  randomizer->SetAttribute("Min", DoubleValue(10));
  randomizer->SetAttribute("Max", DoubleValue(100));

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizer),
                                "Y", PointerValue(randomizer), "Z", PointerValue(randomizer));

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  NodeContainer nodes;
  nodes.Create(numberOfnodes);

  ////////////////
  // 1. Install Wifi
  NetDeviceContainer wifiNetDevices = wifi.Install(wifiPhyHelper, wifiMacHelper, nodes);

  // 2. Install Mobility model
  mobility.Install(nodes);

  // 3. Install NDN stack
  NS_LOG_INFO("Installing NDN stack");
  ndn::StackHelper ndnHelper;
  // ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback
  // (MyNetDeviceFaceCallback));
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(nodes);

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/best-route");

  // Set Energy Model
  BasicEnergySourceHelper basicEnergySourceHelper;
  basicEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));

  EnergySourceContainer sources = basicEnergySourceHelper.Install (nodes);

  WifiRadioEnergyModelHelper wifiRadioEnergyModelHelper;
  wifiRadioEnergyModelHelper.Set ("TxCurrentA", DoubleValue (0.0174));

  DeviceEnergyModelContainer deviceEnergyModels = wifiRadioEnergyModelHelper.Install (wifiNetDevices, sources);
  
  // 4. Set up applications
  NS_LOG_INFO("Installing Applications");

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/test/prefix");
  consumerHelper.SetAttribute("Frequency", DoubleValue(10.0));
  

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/");
  producerHelper.SetAttribute("PayloadSize", StringValue("1200"));
  
  for (uint16_t i = 1; i <= numberOfnodes; i++)
    { 
      auto proapp = producerHelper.Install (nodes.Get (0));
      auto cunapp = consumerHelper.Install (nodes.Get (i));

      cunapp.Start (Seconds (i));
      proapp.Start (Seconds (0.0));
      cunapp.Stop (Seconds (i+1));
      rroapp.Stop (Seconds (30.0));
    }

  Simulator::Stop(Seconds(30.0));

  Simulator::Run();

  for (uint16_t j = 0; j <= numberOfnodes; j++)
    {
      Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (sources.Get (j));
      //basicEnergySource->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
      Ptr<DeviceEnergyModel> basicRadioModels = basicEnergySource->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
      Ptr<WifiRadioEnergyModel> ptr = DynamicCast<WifiRadioEnergyModel> (basicRadioModels);
      NS_ASSERT (basicRadioModels != NULL);
      IdleCurrent = ptr->GetIdleCurrentA ();
      TxCurrent = ptr->GetTxCurrentA ();
      RxCurrent = ptr->GetRxCurrentA ();
      NS_LOG_UNCOND ("Idle Current: " << IdleCurrent);
      NS_LOG_UNCOND ("TX Current: " << TxCurrent);
      NS_LOG_UNCOND ("RX Current: " << RxCurrent);
    }

  basicRadioModels->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));

  for (DeviceEnergyModelContainer::Iterator iter = deviceEnergyModels.Begin (); iter != deviceEnergyModels.End (); iter ++)
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
