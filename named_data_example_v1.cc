/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Connecticut
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Robert Martin <robert.martin@engr.uconn.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"  //may not be needed here...
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"


using namespace ns3;
using ns3::ndn::StrategyChoiceHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::GlobalRoutingHelper;

NS_LOG_COMPONENT_DEFINE("NamedDataExample");

int
main (int argc, char *argv[])
{
  double simStop = 60; //seconds
  uint32_t m_dataRate = 180;
  uint32_t m_packetSize = 32;

  LogComponentEnable ("NamedDataExample", LOG_LEVEL_INFO);

  //to change on the fly
  CommandLine cmd;
  cmd.Parse(argc,argv);

  std::cout << "-----------Initializing simulation-----------\n";

  NodeContainer nodes;
  nodes.Create(50);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodes);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  channel.AddDevice(nodes);

  NamedDataHelper ndHelper;
  ndHelper.SetChannel(channel.Create());
  ndHelper.SetEnergyModel("ns3::AquaSimEnergyModel",
                          "RxPower", DoubleValue(0.1),
                          "TxPower", DoubleValue(2.0),
                          "InitialEnergy", DoubleValue(50),
                          "IdlePower", DoubleValue(0.01));
  
  /*
   * Preset up mobility model for nodes here
   */

  MobilityHelper mobility;
  
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("1000.0"),
                                 "Y", StringValue ("1000.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  PacketSocketAddress socket;
  socket.SetAllDevices();
  socket.SetPhysicalAddress (devices.Get(0)->GetAddress());
  socket.SetProtocol (0);

  OnOffNdHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

  ApplicationContainer apps = app.Install (nodes);
  apps.Start (Seconds (0.0));
  apps.Stop (Seconds (simStop));

  //XXX remove sink assignment here for a correct producer/consumer app model
  Ptr<Node> sNode = nodes.Get(0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sNode, psfid);
  sinkSocket->Bind (socket);

  Packet::EnablePrinting ();  //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop + 1));
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Simulation Completed.\n";
  return 0;
}
