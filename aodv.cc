/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * This is an example script for AODV manet routing protocol. 
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include <iostream>
#include <cmath>
#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

/**
 * \ingroup aodv-examples
 * \ingroup examples
 * \brief Test script.
 * 
 * This script creates 1-dimensional grid topology and then ping last node from the first one:
 * 
 * [10.0.0.1] <-- step --> [10.0.0.2] <-- step --> [10.0.0.3] <-- step --> [10.0.0.4]
 * 
 * ping 10.0.0.4
 */
class AodvExample 
{
public:
  AodvExample ();
  /**
   * \brief Configure script parameters
   * \param argc is the command line argument count
   * \param argv is the command line arguments
   * \return true on successful configuration
  */
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /**
   * Report results
   * \param os the output stream
   */
  void Report (std::ostream & os);

private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;

  // network
  /// nodes used in the example
  NodeContainer nodes;
  NodeContainer serverNode;
  /// devices used in the example
  NetDeviceContainer devices;
  /// interfaces used in the example
  Ipv4InterfaceContainer interfaces;

private:
  /// Create the nodes
  void CreateNodes ();
  /// Create the devices
  void CreateDevices ();
  /// Create the network
  void InstallInternetStack ();
  /// Create the simulation applications
  void InstallApplications ();
};

static void 
CourseChange (std::string context, Ptr<const MobilityModel> position)
{
  Vector pos = position->GetPosition ();
  std::cout << Simulator::Now () << ", pos=" << position << ", x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << std::endl;
}

int main (int argc, char **argv)
{
  AodvExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodvExample::AodvExample () :
  size (9),
  step (100),
  totalTime (100),
  pcap (true),
  printRoutes (true)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  return true;
}

void
AodvExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                  MakeCallback (&CourseChange));

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
AodvExample::Report (std::ostream &)
{ 
}

void
AodvExample::CreateNodes ()
{
  nodes.Create (size-3);
  serverNode.Create (3);

  for (uint16_t i=0; i < 3; i++)
  {
    NodeContainer cm1 = NodeContainer (serverNode.Get (0), nodeds.Get (i));
  }

  for (uint16_t i=3; i < 6; i++)
  {
    NodeContainer cm2 = NodeContainer (serverNode.Get (1), nodeds.Get (i));
  }

  NodeContainer c1 = NodeContainer (serverNode.Get (0), serverNode.Get (2));
  NodeContainer c2 = NodeContainer (serverNode.Get (1), serverNode.Get (2));

  NodeContainer allNodes = NodeContainer (cm1, cm2, serverNode.Get (2));

  // Name nodes
  /*
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  */
  // Create static grid
  MobilityHelper mobility;
  
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("100.0"),
                                 "Y", StringValue ("100.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
  mobility.Install (allNodes);
}

void
AodvExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  //devices = wifi.Install (wifiPhy, wifiMac, nodes); 
  for(uint16_t i=0; i<nodes.size (); i++)
    {
      deviceAdjacencyList[i] = wifi.Install (wifiPhy, wifiMac, nodes);
    }
  for(uint16_t i=0; i<serverNode.size (); i++)
    {
      serverDeviceAdjacencyList[i] = wifi.Install (wifiPhy, wifiMac, serverNode);
    }

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
AodvExample::InstallInternetStack ()
{
  AodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (allNodes);

  Ipv4AddressHelper ipv4;
  std::vector<Ipv4InterfaceContainer> interfaceAdjacencyList (size-3);
  for(uint32_t i=0; i<interfaceAdjacencyList.size (); ++i)
    {
      std::ostringstream cluster_members;
      cluster_members<<"10.1."<<i+1<<".0";
      ipv4.SetBase (cluster_members.str ().c_str (), "255.255.255.0");
      interfaceAdjacencyList[i] = ipv4.Assign (deviceAdjacencyList[i]);
    }
  std::vector<Ipv4InterfaceContainer> interfaceAdjacencyLists (3);
  for(uint32_t i=0; i<interfaceAdjacencyLists.size (); ++i)
    {
      std::ostringstream cluster_head;
      cluster_head<<"10.2."<<i+1<<".0";
      ipv4.SetBase (cluster_head.str ().c_str (), "255.255.255.0");
      interfaceAdjacencyLists[i] = ipv4.Assign (serverDeviceAdjacencyList[i]);
    }
  
  Ipv4Address sinkAddress = interfaceAdjacencyLists[2].GetAddress (2);
  for(uint32_t i=0; i<2; i++)
    {
    Ipv4Address clusterAddress = interfaceAdjacencyLists[i].GetAddress (i);
    }

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
AodvExample::InstallApplications ()
{
  /*
  V4PingHelper ping (interfaces.GetAddress (size - 1));
  ping.SetAttribute ("Verbose", BooleanValue (true));

  ApplicationContainer p = ping.Install (nodes.Get (0));
  p.Start (Seconds (0));
  p.Stop (Seconds (totalTime) - Seconds (0.001));

  // move node away
  Ptr<Node> node = nodes.Get (size/2);
  Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
  Simulator::Schedule (Seconds (totalTime/3), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));
  */

  uint16_t port = 5000;
  for(uint32_t i=0; i<serverNode.size (); i++)
    {
      OnOffHelper onoff_destination ("ns3::UdpSocketFactory", InetSocketAddress (sinkAddress, port));
      onoff_destination.SetConstantRate (DataRate ("400Mb/s"), 1420);
      onoff_destination.SetAttribute ("StartTime", TimeValue (Seconds (0.5)));
      onoff_destination.SetAttribute ("StopTime", TimeValue (Seconds (simuTime)));
      ApplicationContainer apps_destination = onoff_destination.Install (serverNode.Get (i));
    }

  for(uint32_t i=0; i<nodes(size-3); i++)
    {
      OnOffHelper onoff_source ("ns3::UdpSocketFactory", InetSocketAddress (clusterAddress, port));
      onoff_source.SetConstantRate (DataRate ("400Mb/s"), 1420);
      onoff_source.SetAttribute ("StartTime", TimeValue (Seconds (0.5)));
      onoff_source.SetAttribute ("StopTime", TimeValue (Seconds (simuTime)));
      ApplicationContainer apps_source = onoff_source.Install (nodes.Get (i));
    }

  apps_source.Start (Seconds (0.5));
  apps_source.Stop (Seconds (totalTime));
  apps_destination.Start (Seconds (1.0));
  apps_destination.Start (Seconds (totalTime));

}

