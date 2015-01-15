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

#ifndef NDN_EXAMPLES_NDN_REPO_INSERT_H
#define NDN_EXAMPLES_NDN_REPO_INSERT_H
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/nstime.h"

#include "ns3/ndnSIM/ndn.cxx/ndn-api-face.h"
#include "sync-ccnx-wrapper.hpp"
#include <map>
#include <queue>
#include <boost/random.hpp>

namespace ns3 {
namespace ndn {

class RepoInsert : public Application
{
public:
  static TypeId
  GetTypeId();

  RepoInsert();

private:
  void
  SingleInsertion();

  void
  onSingleData(Ptr<const Interest> origInterest, Ptr<const Data> data);

  void
  onSingleTimeout(Ptr<const Interest> origInterest);

  void
  SegmentInsertion();

  void
  SegInit();

  void
  onSegmentData(Ptr<const Interest> interest, Ptr<const Data> data);

  void
  onSegmentTimeout(Ptr<const Interest> interest);

  void
  reSend(Ptr<Interest> origInterest);

  void
  onSyncInterest(Ptr<const Name> prefix, Ptr<const Interest> interest);
  
protected:
  // inherited from Application base class.
  virtual void
  StartApplication();

  virtual void
  StopApplication();
  
private:
  Ptr<ApiFace> m_face;
  //CcnxWrapperPtr m_ccnxHandle;
  std::map<Name, uint64_t > m_retryCount;
  std::queue<uint64_t> m_nextSegmentQueue;
  Name m_name;
  Time m_interestLifetime;
  uint64_t m_nextSegment;
  uint64_t m_credit;
  uint64_t m_startBlockId;
  uint64_t m_endBlockId;
  uint64_t m_receivedData;
  uint64_t m_sendInterest;
  boost::mt19937 m_randomGenerator;
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> > m_random;

};

} // namespace ndn
} // namespace ns3

#endif // NDN_EXAMPLES_NDN_REPO_INSERT_H
