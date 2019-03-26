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

// ndn-grid.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

/**
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-grid
 */

class PcapWriter {
public:
  PcapWriter(const std::string& file)
  {
    PcapHelper helper;
    m_pcap = helper.CreateFile(file, std::ios::out, PcapHelper::DLT_PPP);
  }

  void
  TracePacket(Ptr<const Packet> packet)
  {
    static PppHeader pppHeader;
    pppHeader.SetProtocol(0x0077);

    m_pcap->Write(Simulator::Now(), pppHeader, packet);
  }

private:
  Ptr<PcapFileWrapper> m_pcap;
};

int
main(int argc, char* argv[])
{
  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("10p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating 3x3 topology
  PointToPointHelper p2p;
  PointToPointGridHelper grid(3, 3, p2p);
  grid.BoundingBox(100, 100, 200, 200);

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(2);
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route2");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> producer = grid.GetNode(2, 2);
  
  NodeContainer consumerNodes;
  for (uint16_t u = 0; u < 3; u++)
    {
      for (uint16_t n = 0; n < 3; n++)
        {
          consumerNodes.Add(grid.GetNode(u, n));
          if (u == 2 && n == 2)
            break;
        }
    }

  // Install NDN applications
  std::string prefix = "/prefix";

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix(prefix);
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  consumerHelper.Install(consumerNodes);

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Signature", UintegerValue(100));
  producerHelper.SetAttribute("KeyLocator", StringValue("/unique/key/locator"));
  producerHelper.Install(producer);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix, producer);

  PcapWriter trace("ndn-grid-trace.pcap");
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTx",
                                MakeCallback(&PcapWriter::TracePacket, &trace));

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  ndn::L3RateTracer::InstallAll("wifi-l3-rate-trace.txt", Seconds(0.5));
  //ndn::L2RateTracer::InstallAll("wifi-l2-rate-trace.txt", Seconds(0.5));
  ndn::AppDelayTracer::InstallAll("app-delay-tracer.txt");

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
