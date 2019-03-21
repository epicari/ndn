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

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.WifiExample");

template <int node>
void RemainingEnergyTrace (double oldValue, double newValue)
{
  std::stringstream ss;
  ss << "energy_" << node << ".log";

  static std::fstream f (ss.str ().c_str (), std::ios::out);

  f << Simulator::Now ().GetSeconds () << "s,    remaining energy=" << newValue << std::endl;
}

int
main(int argc, char* argv[])
{
  
  std::string phyMode = "HtMcs7";
  uint16_t numberOfnodes = 10;
  uint16_t sNode = 1;
  double voltage = 3.0;
  double initialEnergy = 7.5;
  double txPowerStart = 0.0;
  double txPowerEnd = 15.0;
  double idleCurrent = 0.273;
  double txCurrent = 0.380;
  double simTime = 10.0;

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

  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Mode", StringValue ("Time"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Time", StringValue ("2s"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  Config::SetDefault ("ns3::RandomWalk2dMobilityModel::Bounds", StringValue ("0|200|0|200"));

  NodeContainer nodes;
  nodes.Create (numberOfnodes);
  
  NodeContainer sinkNode;
  sinkNode.Create (sNode);

  NodeContainer cm1 = NodeContainer (nodes.Get (0), nodes.Get (1), nodes.Get (2), nodes.Get (3), nodes.Get (4));

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

  //wifiMacHelper.SetType("ns3::AdhocWifiMac");
  wifiMacHelper.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid1));
  NetDeviceContainer wifiAPch1 = wifi.Install (wifiPhy, wifiMacHelper, sinkNode.Get (0));

  wifiMacHelper.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid1));
  NetDeviceContainer wifiSTAch1 = wifi.Install (wifiPhy, wifiMacHelper, cm1);

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
  //ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1000");
  ndnHelper.setCsSize(2); // allow just 2 entries to be cached
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll ();

  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/self-learning");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/ncc");
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  EnergySourceContainer eSources;
  BasicEnergySourceHelper basicEnergySourceHelper;
  WifiRadioEnergyModelHelper wifiRadioEnergyModelHelper;
  basicEnergySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (initialEnergy));
  basicEnergySourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (voltage));

  wifiRadioEnergyModelHelper.Set ("TxCurrentA", DoubleValue (txCurrent));
  wifiRadioEnergyModelHelper.Set ("IdleCurrentA", DoubleValue (idleCurrent));

  double eta = DbmToW (txPowerStart) / ((txCurrent - idleCurrent) * voltage);

  wifiRadioEnergyModelHelper.SetTxCurrentModel ("ns3::LinearWifiTxCurrentModel",
                                       "Voltage", DoubleValue (voltage),
                                       "IdleCurrent", DoubleValue (idleCurrent),
                                       "Eta", DoubleValue (eta));

  for (NodeContainer::Iterator n = sinkNode.Begin (); n != sinkNode.End (); n++)
    {
      eSources.Add (basicEnergySourceHelper.Install (*n));

      Ptr<WifiNetDevice> wnd;

      for (uint32_t i = 0; i < (*n)->GetNDevices (); ++i)
        {
          wnd = (*n)->GetDevice (i)->GetObject<WifiNetDevice> ();
          // if it is a WifiNetDevice
          if (wnd != 0)
            {
              // this device draws power from the last created energy source
              wifiRadioEnergyModelHelper.Install (wnd, eSources.Get (eSources.GetN () - 1));
            }
        }
    }

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/test/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("64"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(1.0)));  
  ApplicationContainer proapp = producerHelper.Install (cm1);
  //proapp.Start (Seconds (0.0));
  //proapp.Stop (Seconds (10.0));

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerBatches");
  //consumerHelper.SetAttribute("Batches", StringValue("1s 1 10s 1 20s 1 30s 1"));
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetPrefix("/test/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("1"));
  //consumerHelper.SetAttribute("NumberOfContents", StringValue("1"));
  ApplicationContainer cunapp = consumerHelper.Install (sinkNode.Get (0));
  //cunapp.Start (Seconds (1.0));
  //cunapp.Stop (Seconds (10.0));

  eSources.Get (0)->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergyTrace<0>));

  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  Simulator::Stop(Seconds(simTime + 1));
  Simulator::Run();

  Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (eSources.Get (0));
  Ptr<DeviceEnergyModel> deviceEnergyPtr = basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
  Ptr<WifiRadioEnergyModel> radioEnergyPtr = DynamicCast<WifiRadioEnergyModel> (deviceEnergyPtr);
  double totalEnergy = radioEnergyPtr->GetTotalEnergyConsumption ();
  NS_LOG_UNCOND ("Total energy consumed= " << totalEnergy << "J");

  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
