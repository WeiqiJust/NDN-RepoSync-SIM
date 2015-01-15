/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright(c) 2013 University of California, Los Angeles
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

#include "ndn-repo-insert.h"

NS_LOG_COMPONENT_DEFINE("ndn.Repo");

namespace ns3 {
namespace ndn {

// Necessary if you are planning to use ndn::AppHelper
NS_OBJECT_ENSURE_REGISTERED(RepoInsert);

TypeId
RepoInsert::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ndn::Repo")
    .SetParent<Application>()
    .AddConstructor<RepoInsert>()
    
    .AddAttribute("Prefix","Name of the Interest",
                   StringValue("/"),
                   MakeNameAccessor(&RepoInsert::m_name),
                   MakeNameChecker())
    .AddAttribute("LifeTime", "LifeTime for interest packet",
                   StringValue("2s"),
                   MakeTimeAccessor(&RepoInsert::m_interestLifetime),
                   MakeTimeChecker())
    .AddAttribute("InterestNumber", "Total number of interest wll be sent",
                   UintegerValue(1000),
                   MakeUintegerAccessor (&RepoInsert::m_sendInterest),
                   MakeUintegerChecker<uint64_t> ())
    ;

  return tid;
}

RepoInsert::RepoInsert()
  : m_face(0)
  //, m_ccnxHandle(new CcnxWrapper ())
  , m_credit(12)
  , m_startBlockId(0)
  , m_endBlockId(13)
  , m_receivedData(0)
  , m_randomGenerator(static_cast<unsigned int>(std::time(0)))
  , m_random(m_randomGenerator, boost::uniform_int<>(0,100000000))
{
}

void
RepoInsert::SingleInsertion()
{
  NS_LOG_FUNCTION(this);
  
  Ptr<Interest> interest = Create<Interest>();
  Name interestName = m_name;
  //interestName.append("single").appendSeqNum(m_random());
  interest->SetName(interestName);
  interest->SetInterestLifetime(m_interestLifetime);
  //std::cout<<"send interest name = "<<interestName<<std::endl;
  m_face->ExpressInterest(interest,
                           MakeCallback(&RepoInsert::onSingleData, this),
                           MakeCallback(&RepoInsert::onSingleTimeout, this));
}

void
RepoInsert::onSingleData(Ptr<const Interest> origInterest, Ptr<const Data> data)
{
  NS_LOG_FUNCTION(this << origInterest->GetName() << data->GetName());
  
  m_retryCount.erase(origInterest->GetName());
  m_receivedData++;
  //if (m_receivedData == m_sendInterest)
    //NS_LOG_INFO ("receive data number  = " << m_receivedData);
}

void
RepoInsert::onSingleTimeout(Ptr<const Interest> origInterest)
{
  uint64_t  count = ++m_retryCount[origInterest->GetName()];
  
  if (count <= 4)
  {
     //NS_LOG_FUNCTION(this << "onSingleTimeout " << origInterest->GetName() << " Count = " << count);
     Ptr<Interest> interest = Create<Interest>();
     interest->SetName(origInterest->GetName());
     interest->SetInterestLifetime(m_interestLifetime);
     //m_face->ExpressInterest(interest,
     //                      MakeCallback(&RepoInsert::onSingleData, this),
     //                      MakeCallback(&RepoInsert::onSingleTimeout, this));

     Simulator::Schedule(Seconds (0), &::ns3::ndn::RepoInsert::reSend, this, interest);
  }
  //else
    //NS_LOG_FUNCTION(this << "Single loss !!!");
}

void
RepoInsert::reSend(Ptr<Interest> origInterest)
{
  m_face->ExpressInterest(origInterest,
                          MakeCallback(&RepoInsert::onSingleData, this),
                          MakeCallback(&RepoInsert::onSingleTimeout, this));
}

void
RepoInsert::SegmentInsertion()
{
  //NS_LOG_FUNCTION(this);
  SegInit();
}

void
RepoInsert::SegInit()
{
  uint64_t segment = m_startBlockId;
  Name name = m_name;
  name.append("segment").appendSeqNum(m_random());
  for (; segment < m_startBlockId + m_credit; ++segment) {
    Name segmentName = name;
    segmentName.appendSeqNum(segment);
    Ptr<Interest> interest = Create<Interest>();
    interest->SetName(segmentName);
    interest->SetInterestLifetime(m_interestLifetime);
    m_face->ExpressInterest(interest,
                            MakeCallback(&RepoInsert::onSegmentData, this),
                            MakeCallback(&RepoInsert::onSegmentTimeout, this));
  }

  m_nextSegment = segment;
  m_nextSegmentQueue.push(segment);
}

void
RepoInsert::onSegmentData(Ptr<const Interest> interest, Ptr<const Data> data)
{

  //check whether sent queue empty
  if (m_nextSegmentQueue.empty()) {
    //do not do anything
    return;
  }

  //pop the queue
  uint64_t sendingSegment = m_nextSegmentQueue.front();
  m_nextSegmentQueue.pop();

  //check whether sendingSegment exceeds
  if (sendingSegment > m_endBlockId) {
    //do not do anything
    return;
  }


  m_retryCount.erase(interest->GetName());
  //express the interest of the top of the queue
  Name fetchName(interest->GetName().getPrefix(-1));
  fetchName.appendSeqNum(sendingSegment);
  Ptr<Interest> fetchInterest = Create<Interest>();
  fetchInterest->SetName(fetchName);
  fetchInterest->SetInterestLifetime(m_interestLifetime);
  m_face->ExpressInterest(fetchInterest,
                          MakeCallback(&RepoInsert::onSegmentData, this),
                          MakeCallback(&RepoInsert::onSegmentTimeout, this));

  if (m_retryCount.count(fetchName) == 0) {
    //not found
    m_retryCount[fetchName] = 0;
  }
  else {
    //found
    m_retryCount[fetchName] = m_retryCount[fetchName] + 1;
  }
  //increase the next seg and put it into the queue
  if ((m_nextSegment + 1) <= m_endBlockId) {
    m_nextSegment++;
    m_nextSegmentQueue.push(m_nextSegment);
  }
}

void
RepoInsert::onSegmentTimeout(Ptr<const Interest> interest)
{
  uint64_t  count = ++m_retryCount[interest->GetName()];
  //NS_LOG_FUNCTION(this << "onSegmentTimeout " << interest->GetName() << " Count = " << count);

  //read the retry time. If retry out of time, fail the process. if not, plus
  uint64_t & retryTime = m_retryCount[interest->GetName()];
  if (retryTime > 4) {
    //NS_LOG_FUNCTION(this << "Segment loss !!!");
    return;
  }
  else {
    //Reput it in the queue, retryTime++
    retryTime++;
    Ptr<Interest> retryInterest = Create<Interest>();
    retryInterest->SetName(interest->GetName());
    retryInterest->SetInterestLifetime(m_interestLifetime);
    m_face->ExpressInterest(retryInterest,
                              MakeCallback(&RepoInsert::onSegmentData, this),
                              MakeCallback(&RepoInsert::onSegmentTimeout, this));
  }

}


void
RepoInsert::StartApplication()
{
  m_face = CreateObject<ApiFace>(GetNode());
  //m_ccnxHandle->SetNode (GetNode ());
  //m_ccnxHandle->StartApplication ();

  //m_ccnxHandle->setInterestFilter (m_name.toUri(),
                                   //bind (&RepoInsert::onSyncInterest, this, _1));
  if (GetNode()->GetId() == 0) {
    for (uint64_t i = 0; i < 1; i++)//m_sendInterest; i++) 
    {
      Time delay = Seconds (2);
      Simulator::Schedule(delay, &::ns3::ndn::RepoInsert::SingleInsertion, this);
    }
  }
  Ptr<const Name> dataName = Create<Name>(m_name);
  m_face->SetInterestFilter(dataName, MakeCallback(&RepoInsert::onSyncInterest, this));
  std::cout<<"register prefix = "<<*dataName<<std::endl;
  //Simulator::Schedule(Seconds(0.1), &::ns3::ndn::RepoInsert::SingleInsertion, this);
  //Simulator::Schedule(Seconds(10), &::ns3::ndn::RepoInsert::SegmentInsertion, this);
}

void
RepoInsert::onSyncInterest(Ptr<const Name> prefix, Ptr<const Interest> interest)
{
  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") interest name : " << *prefix);
}

void
RepoInsert::StopApplication()
{
  //NS_LOG_FUNCTION(this);
  m_face->Shutdown();
  m_face = 0;
}

} // namespace ndn
} // namespace ns3

