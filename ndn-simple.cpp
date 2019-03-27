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

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
namespace ns3 {

/**
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

void
CacheEntryRemoved (std::string context, Ptr<const ndn::cs::Entry> entry, Time lifetime)
{
    std::cout << entry->GetName () << " " << lifetime.ToDouble (Time::S) << "s" << std::endl;
}

int
main(int argc, char* argv[])
{
  uint16_t numberOfNodes = 5;
  uint16_t distance = 400;
  uint16_t simTime = 10;

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("10p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(numberOfNodes);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Mb/s")));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(2), nodes.Get(3));
  p2p.Install(nodes.Get(3), nodes.Get(4));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru", "MaxSize", "0"); 
  ndnHelper.InstallAll();

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/best-route");

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  consumerHelper.SetPrefix("/prefix");
  //ApplicationContainer cons = consumerHelper.Install(nodes.Get(0));
  consumerHelper.Install(nodes.Get(0).Start (Seconds (0.0)));
  consumerHelper.Install(nodes.Get(0).Start (Seconds (4.0)));

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  //ndnGlobalRoutingHelper.AddOrigins("/prefix", nodes.Get(4));
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1500"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(4.0)));
  ApplicationContainer prod = producerHelper.Install(nodes.Get(4));

  //cons.Start (Seconds (0.0));
  //prod.Start (Seconds (0.0));

  ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(1.0));
  //ndn::AppDelayTracer::InstallAll("delay-tracer.txt");
  ndn::CsTracer::InstallAll("cs-trace.txt", Seconds(1.0));

    // The failure of the link connecting consumer and router will start
  Simulator::Schedule(Seconds(2.0), ndn::LinkControlHelper::FailLink, nodes.Get(3), nodes.Get(4));
  Simulator::Schedule(Seconds(5.0), ndn::LinkControlHelper::UpLink, nodes.Get(3), nodes.Get(4));

  Config::Connect ("/NodeList/*/$ns3::ndn::cs::Stats::Lru/WillRemoveEntry", MakeCallback (CacheEntryRemoved));

  Simulator::Stop(Seconds(simTime));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
