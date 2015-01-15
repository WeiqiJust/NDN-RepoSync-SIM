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
#include "ns3/internet-module.h"  
#include "ns3/applications-module.h" 
#include "ns3/point-to-point-module.h"   
#include "ns3/ndnSIM-module.h"
#include <iostream>
#include <fstream>
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
 *     NS_LOG=ndn.Consumer:ndn.RepoSyncDelete ./waf --run=ndn-sync-delete
 */

int count=0;

static void RxDrop (Ptr<const Packet> p) 
{  
  //NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());  
  count++;
} 

struct link_info
{
  int src;
  int dst;
};

int 
main (int argc, char *argv[])
{/*
  double dropRate[9] = {0, 0.01, 0.02,0.05, 0.1, 0.15,0.2,0.3,0.4};
  std::string nPacket[2] = {"1000", "10000"};
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 9; j++)
    {*/


  std::ifstream op;
  op.open("/home/justin/topology.txt");
  int linkSize;
  op>>linkSize;
  link_info *topo = new link_info[linkSize];
  std::cout<<"link size  = "<<linkSize<<std::endl;
  for(int k=0;k<linkSize;k++)
  { 
    op>>topo[k].src>>topo[k].dst;
  }
  std::cout<<"--------------------------- simulation begin---------------------------"<<std::endl;
  //std::cout<<"Number of Packets = "<<nPacket[i]<<"      dropRate = "<<dropRate[j]<<std::endl;

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("100Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("5ms"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("200000000"));
  //Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (1));
  //Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create (34);  

  PointToPointHelper p2p;
  NetDeviceContainer pppNetDevice[78];
  for (int k = 0; k<linkSize; k++)
  {
     //std::cout<<k<<std::endl;
     pppNetDevice[k]  = p2p.Install (nodes.Get (topo[k].src - 1), nodes.Get (topo[k].dst - 1));
  }


  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetContentStore ("ns3::ndn::cs::Nocache");
  ndnHelper.SetDefaultRoutes (true);
    //ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
  ndnHelper.SetContentStore ("ns3::ndn::cs::Lru","MaxSize", "100");
  ndnHelper.InstallAll ();

  // Installing applications

  ndn::AppHelper consumerHelper("ns3::ndn::RepoSyncDelete");
  consumerHelper.SetPrefix("/ndn/broadcast");
  std::string str("/creator/");
  for (int k = 1; k<34; k++)
  {
     char *num = new char[3];
     bzero(num, 3);
     sprintf(num,"%d", k);
     std::string name = str + num;
     consumerHelper.SetAttribute("CreatorName", StringValue(name));
     consumerHelper.Install (nodes.Get (k));
  }

  consumerHelper.SetAttribute("Master", StringValue("1"));
  consumerHelper.SetAttribute("CreatorName", StringValue("/creator/0"));
  consumerHelper.Install (nodes.Get (0)); 

 
  Simulator::Stop (Seconds (10));

  Simulator::Run ();
  Simulator::Destroy ();
  //std::cout<<"number of loss = "<<count<<std::endl;
  std::cout<<"--------------------------- simulation end---------------------------"<<std::endl<<std::endl;;
   // }
  

  return 0;
}
