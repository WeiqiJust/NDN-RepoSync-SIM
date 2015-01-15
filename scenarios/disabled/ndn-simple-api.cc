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
 *     NS_LOG=ndn.Consumer:ndn.RepoInsert ./waf --run=ndn-simple-api
 */

int once = 0;
int twice = 0;
int three = 0;
int four = 0;
int five = 0;
static void RxDrop (Ptr<const Packet> p) 
{  
  //NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());  
  if (Simulator::Now ().GetSeconds () < 2)
    once++;
  else if (Simulator::Now ().GetSeconds () < 4)
    twice++;
  else if (Simulator::Now ().GetSeconds () < 6)
    three++;
  else if (Simulator::Now ().GetSeconds () < 8)
    four++;
  else if (Simulator::Now ().GetSeconds () < 10)
    five++;
} 

int 
main (int argc, char *argv[])
{
  double dropRate[2] = {0.5, 0.6};
  std::string nPacket[10] = {"1000","2000", "3000","4000", "5000","6000", "7000","8000", "9000", "10000"};
  for (int i = 0; i < 10; i++)
    for (int j = 0; j < 2; j++)
    {
          once = 0;
          twice =0;
          three = 0;
          four = 0;
          five = 0;
          std::cout<<"--------------------------- simulation begin---------------------------"<<std::endl;
          std::cout<<"Number of Packets = "<<nPacket[i]<<"      dropRate = "<<dropRate[j]<<std::endl;

          // setting default parameters for PointToPoint links and channels
          Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("100Mbps"));
          Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("10ms"));
          Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("200000000"));
          //Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (1));
          //Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

          // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
          CommandLine cmd;
          cmd.Parse (argc, argv);

          // Creating nodes
          NodeContainer nodes;
          nodes.Create (3);  

          // Connecting nodes using two links
          PointToPointHelper p2p;
          NetDeviceContainer pppNetDevice = p2p.Install (nodes.Get (0), nodes.Get (1));
          p2p.Install (nodes.Get (1), nodes.Get (2));

          Ptr<RateErrorModel> em = CreateObjectWithAttributes<RateErrorModel> (  
              "ErrorRate", DoubleValue (0),
              "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));

          pppNetDevice.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue (em));  
          pppNetDevice.Get(1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop)); 

          // Install NDN stack on all nodes
          ndn::StackHelper ndnHelper;
          ndnHelper.SetDefaultRoutes (true);
          ndnHelper.InstallAll ();

          // Installing applications

          // Consumer
          ndn::AppHelper consumerHelper ("ns3::ndn::Repo");
          consumerHelper.SetPrefix ("/prefix/1");
          consumerHelper.SetAttribute("InterestNumber", StringValue(nPacket[i]));
          ApplicationContainer app0 = consumerHelper.Install (nodes.Get (0)); // first node
          //ApplicationContainer app1 = consumerHelper.Install (nodes.Get (2));
          //app0.Stop (Seconds (1000.0));

          // Producer
          ndn::AppHelper producerHelper ("ns3::ndn::Producer");
          // Producer will reply to all requests starting with /prefix
          producerHelper.SetPrefix ("/");
          producerHelper.SetAttribute ("Postfix", StringValue ("/unique/postfix"));
          producerHelper.SetAttribute ("PayloadSize", StringValue("1200"));
          producerHelper.Install (nodes.Get (2)); // last node*/
         
          Simulator::Stop (Seconds (1000.0));

          Simulator::Run ();
          Simulator::Destroy ();
         /*std::cout<<"retransmission once : "<<once<<std::endl;
          std::cout<<"retransmission twice : "<<twice<<std::endl;
          std::cout<<"retransmission three : "<<three<<std::endl;
          std::cout<<"retransmission four : "<<four<<std::endl;
          std::cout<<"did not get : "<<five<<std::endl;
          std::cout<<"--------------------------- simulation end---------------------------"<<std::endl<<std::endl;;*/
    }
  

  return 0;
}
