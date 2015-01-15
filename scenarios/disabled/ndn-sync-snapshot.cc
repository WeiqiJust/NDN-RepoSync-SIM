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
 *     NS_LOG=ndn.Consumer:ndn.RepoSyncSnapshot ./waf --run=ndn-sync-snapshot
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
{
  std::ifstream op1, op2;
  op1.open("/home/justin/topology.txt");
  op2.open("/home/justin/topo_append.txt");
  int linkSize, appendLink;
  op1>>linkSize;
  link_info *topo = new link_info[linkSize];
  std::cout<<"link size  = "<<linkSize<<std::endl;
  op2>>appendLink;
  link_info *topoAppend = new link_info[appendLink];
  for(int k=0;k<linkSize;k++)
  { 
    op1>>topo[k].src>>topo[k].dst;
  }
  for(int k=0;k<appendLink;k++)
  { 
    op2>>topoAppend[k].src>>topoAppend[k].dst;
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
  nodes.Create (54);  

  PointToPointHelper p2p;
  for (int k = 0; k<linkSize; k++)
  {
     //std::cout<<k<<std::endl;
     p2p.Install (nodes.Get (topo[k].src - 1), nodes.Get (topo[k].dst - 1));
  }
  
  uint64_t startTime = 100;
  for (int k = 0; k<appendLink; k++)
  {
     //std::cout<<k<<std::endl;
     p2p.Install (nodes.Get (topoAppend[k].src - 1), nodes.Get (topoAppend[k].dst - 1));
  }



  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetContentStore ("ns3::ndn::cs::Nocache");
  ndnHelper.SetDefaultRoutes (true);
    //ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
  ndnHelper.SetContentStore ("ns3::ndn::cs::Lru","MaxSize", "100");
  ndnHelper.InstallAll ();

  // Installing applications

  ndn::AppHelper consumerHelper("ns3::ndn::RepoSyncSnapshot");
  consumerHelper.SetPrefix("/ndn");
  std::string str("/creator/");
  
  consumerHelper.SetAttribute("Master", StringValue("1"));
  consumerHelper.SetAttribute("CreatorName", StringValue("/creator/0"));
  consumerHelper.Install (nodes.Get (0)); 

  consumerHelper.SetAttribute("Master", StringValue("0"));
  for (int k = 1; k<54; k++)
  {
     char *num = new char[3];
     bzero(num, 3);
     sprintf(num,"%d", k);
     std::string name = str + num;
     if (k > 33)
       consumerHelper.SetAttribute("Start", StringValue("100000"));
     else
       consumerHelper.SetAttribute("Start", StringValue("0"));
     consumerHelper.SetAttribute("CreatorName", StringValue(name));
     consumerHelper.Install (nodes.Get (k));
  }


  for (int k = 0; k<appendLink; k++)
  {
     //std::cout<<k<<std::endl;
     Simulator::Schedule (Seconds (0), ndn::LinkControlHelper::FailLink, nodes.Get (topoAppend[k].src - 1), nodes.Get (topoAppend[k].src - 1));
     Simulator::Schedule (Seconds (startTime), ndn::LinkControlHelper::UpLink, nodes.Get (topoAppend[k].src - 1), nodes.Get (topoAppend[k].src - 1));
  }
 

  Simulator::Stop (Seconds (200));

  Simulator::Run ();
  Simulator::Destroy ();
  //std::cout<<"number of loss = "<<count<<std::endl;
  std::cout<<"--------------------------- simulation end---------------------------"<<std::endl<<std::endl;;

  return 0;
}
