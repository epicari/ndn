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

int
main(int argc, char* argv[])
{
  uint16_t numberOfRouters = 2;
  uint16_t numberOfPeers = 3;
  uint16_t distance = 2000;
  uint16_t simTime = 20.1;

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("10p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer routers;
  routers.Create (numberOfRouters);

  NodeContainer peers;
  peers.Create (numberOfPeers);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Mb/s")));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.030)));
  p2p.Install(peers.Get(0), routers.Get(0));
  p2p.Install(routers.Get(0), routers.Get(1));
  //p2p.Install(routers.Get(1), routers.Get(2));
  p2p.Install(routers.Get(1), peers.Get(1));
  p2p.Install(peers.Get(2), routers.Get(0));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Freshness::Lru", "MaxSize", "10000"); 
  ndnHelper.InstallAll();

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  for (uint16_t i = 0; i < 5; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }

  //positionAlloc->Add (Vector(800, 0, 0));
  
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  //mobility.Install (routers);
  mobility.InstallAll ();
  /*
  positionAlloc->Add (Vector(0, 0, 0));
  positionAlloc->Add (Vector(500800, 0, 0));
  positionAlloc->Add (Vector(0, 0, 0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install (peers);
  */
  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/video_01", "/localhost/nfd/strategy/best-route");

  // Installing applications

  ApplicationContainer cons;
  ApplicationContainer prod;

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  consumerHelper.SetPrefix("/video_01");
  cons.Add (consumerHelper.Install(peers.Get(0)));
  cons.Start (Seconds (0.0));
  cons.Stop (Seconds (10.0));
  cons.Add (consumerHelper.Install(peers.Get(2)));
  cons.Start (Seconds (10.1));

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1500"));
  producerHelper.SetPrefix("/video_01");
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds (20.1)));
  prod.Add (producerHelper.Install(peers.Get(1)));

  ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds (1.0));
  ndn::CsTracer::InstallAll("cs-trace.txt", Seconds (1.0));
  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");

  ndn::GlobalRoutingHelper::CalculateRoutes();

    // The failure of the link connecting consumer and router will start
  //Simulator::Schedule(Seconds(2.0), ndn::LinkControlHelper::FailLink, nodes.Get(3), nodes.Get(4));
  //Simulator::Schedule(Seconds(5.0), ndn::LinkControlHelper::UpLink, nodes.Get(3), nodes.Get(4));

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
