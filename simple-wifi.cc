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
#include "ns3/dsdv-module.h"

using namespace std;
namespace ns3 {

Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;

static inline std::string
PrintReceivedPacket (Address& from)
{
  InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (from);

  std::ostringstream oss;
  oss << "--\nReceived one packet! Socket: " << iaddr.GetIpv4 ()
      << " port: " << iaddr.GetPort ()
      << " at time = " << Simulator::Now ().GetSeconds ()
      << "\n--";

  return oss.str ();
}

void
ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          NS_LOG_UNCOND (PrintReceivedPacket (from));
        }
    }
}

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

template <int node>
void RemainingEnergyTrace (double oldValue, double newValue)
{
  std::stringstream ss;
  ss << "tcp_ip_energy_" << node << ".log";

  static std::fstream f (ss.str ().c_str (), std::ios::out);

  f << Simulator::Now ().GetSeconds () << "s,    remaining energy=" << newValue << std::endl;
}

int
main(int argc, char* argv[])
{
  
  std::string phyMode = "HtMcs7";
  std::string tcpVariant = "TcpNewReno";
  uint16_t port = 1234;
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
  
  tcpVariant = std::string ("ns3::") + tcpVariant;
  // Select TCP variant
  if (tcpVariant.compare ("ns3::TcpWestwoodPlus") == 0)
    {
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));
    }

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (64));

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

  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer inetface = address.Assign (wifiSTAch1);
  address.SetBase ("10.10.1.0", "255.255.255.0");
  Ipv4InterfaceContainer inetfaceSink = address.Assign (wifiAPch1);
  Ipv4Address inetAddr = inetfaceSink.GetAddress (0);

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

  PacketSinkHelper producerHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApp = producerHelper.Install (sinkNode.Get (0));
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  //UdpServerHelper producerHelper (port);
  //producerHelper.Install (nodes.Get (0));

  //UdpClientHelper consumerHelper (inetAddr, port);
//  BulkSendHelper consumerHelper ("ns3::TcpSocketFactory", InetSocketAddress (inetAddr, port));
//  consumerHelper.SetAttribute ("MaxBytes", UintegerValue (1000));
//  consumerHelper.SetAttribute ("SendSize", UintegerValue (64));
//  consumerHelper.Install (nodes);

  OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (inetAddr, port)));
  server.SetAttribute ("PacketSize", UintegerValue (64));
  server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  //server.Install (nodes);
  ApplicationContainer serverApp = server.Install (nodes);
  serverApp.Start (Seconds (1.0));

  eSources.Get (0)->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergyTrace<0>));
  
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);
  Simulator::Stop(Seconds(simTime + 1));
  Simulator::Run();
  
  double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * simTime));
  std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;

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
