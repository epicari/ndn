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
#include "ns3/energy-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NamedDataExample");

class NDaqua
{
  public:
    void Run();
    void ReceivedPkt(Ptr<Socket> socket);
};

void
NDaqua::ReceivedPkt(Ptr<Socket> socket)
{
  NS_LOG_DEBUG("Test, recv a packet");
  Ptr<Packet> packet;
  while ((packet = socket->Recv ()))
  {
    NS_LOG_DEBUG("Recv a packet of size " << packet->GetSize());
  }
}

void
NDaqua::Run()
{
  double simStop = 60; //seconds
  double rxPower = 0.1;
  double txPower = 2.0;
  double initialEnergy = 50;
  double idlePower = 0.01;
  //double energyTx = 0.0;
  //double energyRx = 0.0;
  uint32_t m_dataRate = 180;
  uint32_t m_packetSize = 32;
  uint32_t numberOfnodes = 50;

  LogComponentEnable ("NamedDataExample", LOG_LEVEL_INFO);

  std::cout << "-----------Initializing simulation-----------\n";

  NodeContainer nodes;
  nodes.Create(numberOfnodes);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodes);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");

  NamedDataHelper ndHelper;
  ndHelper.SetChannel(channel.Create());
  ndHelper.SetEnergyModel("ns3::AquaSimEnergyModel",
                          "RxPower", DoubleValue (rxPower),
                          "TxPower", DoubleValue (txPower),
                          "InitialEnergy", DoubleValue (initialEnergy),
                          "IdlePower", DoubleValue (idlePower));
  
  MobilityHelper mobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  Vector boundry = Vector(0,0,0);

  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodes.Begin(); i != nodes.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);
      devices.Add(ndHelper.Create(*i, newDevice));
/*
      NS_LOG_DEBUG("Node: " << *i << " newDevice: " << newDevice << " Position: " <<
		     boundry.x << "," << boundry.y << "," << boundry.z <<
		     " freq:" << newDevice->GetPhy()->GetFrequency() << " addr:" <<
         AquaSimAddress::ConvertFrom(newDevice->GetAddress()).GetAsInt() );
*/
      boundry.x += 10;
      boundry.y += 10;
      boundry.z += 10;
    }

  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodes);

  PacketSocketAddress socket;
  socket.SetAllDevices ();
  socket.SetPhysicalAddress (devices.Get (0)->GetAddress());
  socket.SetProtocol (0);

  Ptr<Node> sinkNode = nodes.Get (0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);


/*
  BasicEnergySourceHelper basicEnergySource;
  basicEnergySource.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (50));
  EnergySourceContainer energySource = basicEnergySource.Install (nodes);
*/

  OnOffNdHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

  for (uint16_t i = 1; i < numberOfnodes; i++)
    {
      ApplicationContainer apps = app.Install (nodes.Get (i));
      sinkSocket->SetRecvCallback (MakeCallback (&NDaqua::ReceivedPkt, this));
      //Ptr<BasicEnergySource> basicEnergySource = DynamicCast<BasicEnergySource> (energySource.Get (i));
      Ptr<AquaSimEnergyModel> aquaEnergy = DynamicCast<AquaSimEnergyModel> (nodes.Get (i));
      
      //std::cout << "Decr Rcv Energy: " << aquaEnergy->DecrRcvEnergy(rxPower);
      //std::cout << "Decr Tx Energy: " << aquaEnergy->DecrTxEnergy(txPower);
      apps.Start (Seconds (i));
      apps.Stop (Seconds (i+0.1));
    }




  Packet::EnablePrinting ();  //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop + 1));
  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Simulation Completed.\n";
}

int
main (int argc, char *argv[])
{
  //to change on the fly
  CommandLine cmd;
  cmd.Parse(argc,argv);

  NDaqua NDaqua;
  NDaqua.Run();
  return 0;
}
