/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */
// ndn-simple.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndn-link-control-helper.h"
using namespace ns3;

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.RepoSync ./waf --run=ndn-sync
 */


static void RxDrop (Ptr<const Packet> p) 
{  
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());  

} 

int 
main (int argc, char *argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1000Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("200000"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create (6);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  NetDeviceContainer pppNetDevice[5];
  pppNetDevice[0] = p2p.Install (nodes.Get (0), nodes.Get (1));
  pppNetDevice[1] = p2p.Install (nodes.Get (1), nodes.Get (2));
  pppNetDevice[2] = p2p.Install (nodes.Get (0), nodes.Get (3));
  pppNetDevice[3] = p2p.Install (nodes.Get (3), nodes.Get (4));
  pppNetDevice[4] = p2p.Install (nodes.Get (2), nodes.Get (5));

  Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> (  
              "ErrorRate", DoubleValue (0),
              "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
  for (int i=0; i<5; i++)
  {
    pppNetDevice[i].Get(1)->SetAttribute("ReceiveErrorModel", PointerValue (em));  
    pppNetDevice[i].Get(1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop)); 
  }


  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.SetContentStore ("ns3::ndn::cs::Nocache");
  ndnHelper.InstallAll ();


  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::RepoSync");
  consumerHelper.SetPrefix("/ndn/broadcast");
  consumerHelper.SetAttribute("CreatorName", StringValue("/creator/1"));
  consumerHelper.Install(nodes.Get(1));
  consumerHelper.SetAttribute("CreatorName", StringValue("/creator/3"));
  consumerHelper.Install(nodes.Get (3));
  consumerHelper.SetAttribute("CreatorName", StringValue("/creator/4"));
  consumerHelper.Install(nodes.Get (4));
    consumerHelper.SetAttribute("CreatorName", StringValue("/creator/2"));
  consumerHelper.Install(nodes.Get (2));


  consumerHelper.SetAttribute("Master", StringValue("1"));
  consumerHelper.SetAttribute("CreatorName", StringValue("/creator/0"));
  consumerHelper.Install (nodes.Get (0)); 

  consumerHelper.SetAttribute("Start", StringValue("70"));
  consumerHelper.SetAttribute("Master", StringValue("0"));
  consumerHelper.SetAttribute("CreatorName", StringValue("/creator/5"));
  consumerHelper.Install (nodes.Get (5)); 


  Simulator::Schedule (Seconds (0), ndn::LinkControlHelper::FailLink, nodes.Get (2), nodes.Get (5));
  Simulator::Schedule (Seconds (71), ndn::LinkControlHelper::UpLink,   nodes.Get (2), nodes.Get (5)); 
  Simulator::Stop (Seconds (100));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
