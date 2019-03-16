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

  NodeContainer nodes;
  nodes.Create(numberOfnodes);

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

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfnodes; i++)
    {
      positionAlloc->Add (Vector(5 * i, 2 * i, 0));
    }

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);

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
  //ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/best-route");

  // Set Energy Model
  BasicEnergySourceHelper basicEnergySourceHelper;
  basicEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (0.1));
  EnergySourceContainer sources = basicEnergySourceHelper.Install (nodes);

  WifiRadioEnergyModelHelper wifiRadioEnergyModelHelper;
  DeviceEnergyModelContainer deviceEnergyModels = wifiRadioEnergyModelHelper.Install (wifiNetDevices, sources);
  
  // 4. Set up applications
  /*
  NS_LOG_INFO("Installing Applications");
  
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/");
  producerHelper.SetAttribute("PayloadSize", StringValue("64"));
  auto proapp = producerHelper.Install (nodes.Get (0));

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/test/prefix");
  auto cunapp = consumerHelper.Install (nodes);

  cunapp.Start (Seconds (0.0));
  cunapp.Stop (Seconds (30.0));
  proapp.Start (Seconds (0.0));
  proapp.Stop (Seconds (30.0));      
  */
  PacketSocketAddress socket;
  socket.SetAllDevices ();
  socket.SetPhysicalAddress (wifiNetDevices.Get (0)->GetAddress ());
  socket.SetProtocol (0);

  for (uint16_t i = 0; i <= numberOfnodes; i++)
    {
      TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
      Ptr<Socket> recvSink = Socket::CreateSocket (nodes.Get (0), tid);
      recvSink->Bind (socket);

      OnOffHelper onoff ("ns3::PacketSocketFacotry", Address (socket));
      onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      onoff.SetAttribute ("PacketSize", UintegerValue (64));
      ApplicationContainer apps = onoff.Install (nodes.Get (u));
      
      apps.Start (Seconds (0.0));
      apps.Stop (Seconds (30.0));
    }

  Simulator::Stop(Seconds(30.0));
  Simulator::Run();

//  for (uint16_t i = 0; i <= numberOfnodes; i++)
//    {
  Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (sources.Get (0));
      //basicEnergySource->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
  Ptr<DeviceEnergyModel> basicRadioModels = basicEnergySource->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
  Ptr<WifiRadioEnergyModel> ptr = DynamicCast<WifiRadioEnergyModel> (basicRadioModels);

  NS_ASSERT (basicRadioModels != NULL);
//    }
r
  //basicRadioModels->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));

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
