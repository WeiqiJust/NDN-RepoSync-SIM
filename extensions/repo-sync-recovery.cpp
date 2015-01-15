/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
* Copyright (c) 2014, Regents of the University of California.
*
* This file is part of NDN repo-ng (Next generation of NDN repository).
* See AUTHORS.md for complete list of repo-ng authors and contributors.
*
* repo-ng is free software: you can redistribute it and/or modify it under the terms
* of the GNU General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* repo-ng is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* repo-ng, e.g., in COPYING.md file. If not, see <http://www.gnu.org/licenses/>.
*/

#include "repo-sync-recovery.hpp"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include "sync-log.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
NS_LOG_COMPONENT_DEFINE("ndn.RepoSyncRecovery");

namespace ns3 {
namespace ndn {



using namespace std;
using namespace boost;
using namespace Sync;



const int syncResponseFreshness = 1000;
const int syncInterestReexpress = 4;
const int defaultRecoveryRetransmitInterval = 200; // milliseconds
const int retrytimes = 4;
const int pipeline = 12;

static bool
compareDigest(std::pair<DigestPtr, ActionEntry> entry, DigestConstPtr digest)
{
  return *entry.first == *digest;
}

static bool
compareActionEntry(std::pair<DigestPtr, ActionEntry> entry, const Name& name)
{
  return entry.second.getName() == name;
}

static bool
compareSnapshot(std::pair<Name, uint64_t> entry, std::pair<Name, uint64_t> info)
{
  return entry == info;
}

static bool
compareSeq(ActionEntry test, uint64_t seq)
{
  return test.getSeqNo() == seq;
}

RepoSyncRecovery::RepoSyncRecovery()
  : m_seq(0)    // action sequence initiate as 0, the first action sequence is 1
  , m_isSynchronized(false)
  //, m_ccnxHandle(new CcnxWrapper ())
  , m_ccnxHandle(new CcnxWrapper ())
  , m_recoveryRetransmissionInterval(defaultRecoveryRetransmitInterval)
  , m_randomGenerator(static_cast<unsigned int>(std::time(0)))
  , m_rangeUniformRandom(m_randomGenerator, boost::uniform_int<>(200,1000))
  , m_reexpressionJitter(m_randomGenerator, boost::uniform_int<>(100,500))
  , m_senddataJitter(m_randomGenerator, boost::uniform_int<>(100,200))
  , m_syncInterestTable(ns3::Seconds(syncInterestReexpress))
  , m_snapshot(SyncStateMsg::SNAPSHOT)
  , m_snapshotNo(0)
  , m_size(0)
  , m_count(0)
  , preSeq(1)
{
  init();
}

RepoSyncRecovery::~RepoSyncRecovery()
{
  m_scheduler.cancel(REEXPRESSING_INTEREST);
}

void
RepoSyncRecovery::init()
{
  m_actionList.clear();
  Name rootName("/");
  ActionEntry entry(rootName, -1);
  m_actionList.push_back(std::make_pair(m_syncTree.getDigest(), entry));
  createSnapshot();
}

Action
RepoSyncRecovery::strToAction(const std::string& action)
{
  if (action == "insertion") {
   return INSERTION;
  }
  else if (action == "deletion") {
    return DELETION;
  }
  else {
    throw Error("Action type is wrong. No sucn action!");
  }
  return OTHERS;
}


void
RepoSyncRecovery::StartApplication()
{
  m_scheduler.schedule(ns3::MilliSeconds(m_start), 
                         bind(&RepoSyncRecovery::start, this),
                         START);
}

void
RepoSyncRecovery::StopApplication()
{
  m_ccnxHandle->clearInterestFilter (m_syncPrefix.toUri());
  m_ccnxHandle->StopApplication ();
  m_scheduler.cancel(REEXPRESSING_INTEREST);
  m_scheduler.cancel(DELAYED_INTEREST_PROCESSING);
}

void
RepoSyncRecovery::start()
{
  //m_creatorName.append(m_master);
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") m_master : " << m_master<<"  creator name = "<<m_creatorName);
  if (m_master != "0")
  {
    std::string file = "/home/justin/generator_";
    file = file + m_master + ".txt";
    std::cout<<"file name" <<file<<std::endl;
    std::ifstream op(file.c_str());
    int fileSize;
    op>>fileSize;
    std::cout<<"size  = "<<fileSize<<std::endl;
    readfile *read = new readfile[fileSize];
    for(int k=0;k<fileSize;k++)
    { 
      op>>read[k].time>>read[k].count>>read[k].frequency;
      std::cout<<"time "<<read[k].time<<" count "<<read[k].count<<" frequency "<<read[k].frequency<<std::endl;
      read[k].time = read[k].time * 1000;
      read[k].frequency = read[k].frequency;
    }
    for (int k=0;k<fileSize;k++)
    {
      for (int i = 0; i< read[k].count; i++)
      {
         Name dataName("/data");
         dataName.append(m_master).appendSeqNum(k).appendSeqNum(i);
         m_scheduler.schedule(ns3::MilliSeconds(read[k].time), bind(&RepoSyncRecovery::insertAction, this, dataName, "insertion"), GENERATE_ACTION);
         read[k].time = read[k].time + read[k].frequency;
      }
    }

    m_file[0].open("/home/justin/node0.txt", ios::out);
    m_file[1].open("/home/justin/node1.txt",ios::out);
    m_file[2].open("/home/justin/node2.txt",ios::out);
    m_file[3].open("/home/justin/node3.txt",ios::out);
    
    /*uint64_t fail = 101000;
    uint64_t uplink = 201000;
    for(uint64_t k=0; k<50; k++)
    { 
      
      Name dataName("/data");
      dataName.append(m_master).appendSeqNum(k);
      //std::cout<<"data name "<<dataName<<std::endl;
      m_scheduler.schedule(ns3::MilliSeconds(time), bind(&RepoSyncRecovery::insertAction, this, dataName, "insertion"), GENERATE_ACTION);
      //time = time + 500;
      Name seperate = dataName;
      seperate.append("/1");
      m_scheduler.schedule(ns3::MilliSeconds(fail), bind(&RepoSyncRecovery::insertAction, this, seperate, "insertion"), GENERATE_ACTION);
      fail = fail + 500;
      Name merge = dataName;
      merge.append("/2");
      m_scheduler.schedule(ns3::MilliSeconds(uplink), bind(&RepoSyncRecovery::insertAction, this, merge, "insertion"), GENERATE_ACTION);
      uplink = uplink + 500;
    }*/
     m_scheduler.schedule(ns3::MilliSeconds(4010), bind(&RepoSyncRecovery::processPendingSyncInterests, this), 102);
    
  }

  m_ccnxHandle->SetNode (GetNode ());
  m_ccnxHandle->StartApplication ();
  std::string str = "/";
  m_ccnxHandle->setInterestFilter (str,
                                   bind(&RepoSyncRecovery::onSyncInterest, this, _1),
                                   bind(&RepoSyncRecovery::setFilterTimeout, this, _1));
  
  m_scheduler.schedule(ns3::MilliSeconds(0), 
                       bind(&RepoSyncRecovery::sendSyncInterest, this),
                       REEXPRESSING_INTEREST);

    
  m_scheduler.schedule(ns3::Seconds(4), bind(&RepoSyncRecovery::printDistribution, this), 103);
  //m_scheduler.schedule(ns3::Seconds(9), bind(&RepoSyncRecovery::removeActions, this), 105);
  m_scheduler.schedule(ns3::Seconds(6), bind(&RepoSyncRecovery::sendSyncInterest, this), 104);
  m_scheduler.schedule(ns3::Seconds(8), bind(&RepoSyncRecovery::sendSyncInterest, this), 104);
  m_scheduler.schedule(ns3::Seconds(10), bind(&RepoSyncRecovery::sendSyncInterest, this), 104);
  //m_scheduler.schedule(ns3::Seconds(50), bind(&RepoSyncRecovery::removeIndexEntry, this), GENERATE_SNAPSHOT);
}

void
RepoSyncRecovery::stop()
{
  m_ccnxHandle->clearInterestFilter (m_syncPrefix.toUri());
  m_scheduler.cancel(REEXPRESSING_INTEREST);
  m_scheduler.cancel(DELAYED_INTEREST_PROCESSING);
  for (int i=0; i<4; i++)
    m_file[i].close();
}

NS_OBJECT_ENSURE_REGISTERED(RepoSyncRecovery);

TypeId
RepoSyncRecovery::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::RepoSyncRecovery")
    .SetParent<Application>()
    .AddConstructor<RepoSyncRecovery>()

    .AddAttribute("Prefix", "Prefix of the Interest",
                   StringValue("/ndn/broadcast"),
                   MakeNameAccessor(&RepoSyncRecovery::m_syncPrefix),
                   MakeNameChecker())
    .AddAttribute("CreatorName","The name of this repo",
                   StringValue("/data"),
                   MakeNameAccessor(&RepoSyncRecovery::m_creatorName),
                   MakeNameChecker())
    .AddAttribute("LifeTime", "LifeTime for interest packet",
                   StringValue("5s"),
                   MakeTimeAccessor(&RepoSyncRecovery::m_interestLifetime),
                   MakeTimeChecker())
    .AddAttribute("Master", "Generate Data",
                   StringValue("0"),
                   MakeStringAccessor(&RepoSyncRecovery::m_master),
                   MakeStringChecker())
    .AddAttribute("Start", "The time to start sync",
                   UintegerValue (0),
                   MakeUintegerAccessor(&RepoSyncRecovery::m_start),
                   MakeUintegerChecker<uint32_t> ())
    ;
  
  return tid;
}

// insert action into actionlist from repo handles
void
RepoSyncRecovery::insertAction(const Name& dataName, const std::string& str)
{
  
  m_seq++;
  Action action = strToAction(str);
  ActionEntry entry(m_creatorName, dataName, action);
  uint64_t version = ++m_seqIndex[std::make_pair(dataName, action)];
  entry.setVersion(version);
  entry.setSeqNo(m_seq);
  entry.constructName();
  m_syncTree.update(entry);
  m_actionList.push_back(std::make_pair(m_syncTree.getDigest(), entry));
  m_nodeSeq[m_creatorName].current = m_seq;
  m_nodeSeq[m_creatorName].final = m_seq;
  std::map<Name, status>::iterator it = m_storageHandle.find(dataName);
  if (str == "insertion")
  {
    if (it == m_storageHandle.end())
      m_storageHandle[dataName] = EXISTED;
    else if (m_storageHandle[dataName] == DELETED) 
      m_storageHandle[dataName] = INSERTED;
  }
  else
  {
    if (it != m_storageHandle.end() && it->second == EXISTED)
      m_storageHandle[dataName] = DELETED;    
  }
  //m_scheduler.schedule(ns3::MilliSeconds(0.01), bind(&RepoSyncRecovery::processPendingSyncInterests, this), 102);
  
}

void
RepoSyncRecovery::printDistribution()
{
  uint64_t size = m_storageHandle.size();
  
  /*
  if (m_actionList.size() > preSeq)
  {
    
    NS_LOG_INFO ("node("<< GetNode()->GetId() <<") "<<m_count*20<<" ms cumulate "<<(m_actionList.size() - preSeq)<<" action");
    preSeq = m_actionList.size();
  }*/
  if (size != 0)
  {
   
    NS_LOG_INFO ("node("<< GetNode()->GetId() <<") "<<size<<" data");
    m_file[GetNode()->GetId()]<<4+0.1*m_count<<" "<<size<<std::endl;
    m_size = size;
    
  }
   m_count++;
   m_scheduler.schedule(ns3::MilliSeconds(100), bind(&RepoSyncRecovery::printDistribution, this), 103);
}

void
RepoSyncRecovery::printSyncStatus(boost::function< void (const Name &, const uint64_t &) > f)
{
  for (SyncTree::const_iter iter = m_syncTree.begin(); iter != m_syncTree.end(); iter++) {
    f(iter->first, iter->second.last);
  }
}

uint64_t
RepoSyncRecovery::printSyncStatus(const Name& name)
{
  SyncTree::const_iter iterator = m_syncTree.lookup(name);
  if (iterator != m_syncTree.end())
    return iterator->second.last;
  else
    return 0;
}

void
RepoSyncRecovery::onSyncInterest(const std::string &str)
{
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") interest name : " << m_storageHandle.size());
  //std::cout<<"on sync interest"<<std::endl;
  Name name(str);
  Name dataName("/data");
  if (dataName.isPrefixOf(name))
  {
    responseData(name);
    return;
  }
  int nameLengthDiff = name.size() - m_syncPrefix.size();
  // syncInterest /ndn/broadcast/sync/digest
  // fetchInterest /ndn/broadcast/fetch/creatorName/seq
  // recoveryInterest /ndn/broadcast/recovery/digest
  BOOST_ASSERT(nameLengthDiff > 1);
  try
    {
      std::string type = name[m_syncPrefix.size()].toUri();
      if (type == "sync")
        {
          DigestConstPtr digest = convertNameToDigest(name);
          processSyncInterest(name, digest, false);
        }
      else if (type == "fetch")
        {
          processFetchInterest(name);
        }
      else if (type == "recovery")
        {
          //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") receive recovery intereest "<<name);
          DigestConstPtr digest = convertNameToDigest(name);
          processRecoveryInterest(name, digest);
        }
       else
         {
           throw Error("The interest type is not supported!");
         }
    }
  catch(ns3::ndn::Error::DigestCalculationError &e)
    {
      throw Error("Something fishy happened...");
      return ;
    }
}

void
RepoSyncRecovery::responseData(const Name& prefix)
{
  
  std::map<Name, status>::iterator it = m_storageHandle.find(prefix);
  ////NS_LOG_INFO ("node("<< GetNode()->GetId() <<") data status : "<<prefix<<" status "<<it->second);
  if (it != m_storageHandle.end() && (it->second == EXISTED || it->second == INSERTED))
  {
    char *wireData = "1234567890";
    m_ccnxHandle->publishRawData (prefix.toUri(), wireData, 10, 100); 
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") responseData name : "<<prefix);
  }
}

DigestConstPtr
RepoSyncRecovery::convertNameToDigest(const Name &name)
{
  BOOST_ASSERT(m_syncPrefix.isPrefixOf(name));

  std::string hash = name.get(-1).toUri();

  DigestPtr digest = boost::make_shared<Digest>();
  std::istringstream is(hash);
  is >> *digest;

  return digest;
}

void
RepoSyncRecovery::processSyncInterest(const Name& name, DigestConstPtr digest, bool timeProcessing)
{
  DigestConstPtr rootDigest = m_syncTree.getDigest();
  //if (GetNode()->GetId() == 0)
  //std::cout<<m_creatorName<<" process sync interest m_digest = "<<*rootDigest<<" received digest = "<<*digest<<std::endl;
  //if (GetNode()->GetId() == 0)
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process sync interest name "<<name);
  if (*rootDigest == *digest) {
    if (!m_isSynchronized) {
      m_scheduler.cancel(SYNCHRONIZED);
      //remove actions when no different digest received for a while
      m_scheduler.schedule(ns3::Seconds(20), bind(&RepoSyncRecovery::removeActions, this), SYNCHRONIZED);
      //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") synced digest "<<*rootDigest);
      m_isSynchronized = true;
    }
    m_syncInterestTable.insert(digest, name.toUri(), false);
    return;
  }
  // if received a different digest, cancel the event of removeActions
  m_scheduler.cancel(SYNCHRONIZED);
  m_isSynchronized = false;
  std::list<std::pair<DigestPtr, ActionEntry> >::iterator it = std::find_if(m_actionList.begin(),
                                                                            m_actionList.end(),
                                                                            bind(&compareDigest, _1, digest));
  // if the digest can be recognized, it means that the digest of the sender repo is outdated
  // return all the missing actions to the sender repo so that it can start to fetch the actions
  if (it != m_actionList.end()) {
    Msg message(SyncStateMsg::ACTION);
    ++it;
    while (it != m_actionList.end()) {
      message.writeActionNameToMsg(it->second);
      ++it;
    }
    //m_scheduler.schedule(ns3::MilliSeconds(m_senddataJitter()), bind(&RepoSyncRecovery::sendData, this, name, message), 100);
    sendData(name, message);
    checkInterestSatisfied(name);
    return;
  }
  // if the digest cannot be recognized, wait for a period of time to process this sync interest since
  // actions may be on the way to this repo
  // if after a period of time, the digest is still unrecognized, go to recovery process
  if (!timeProcessing)
    {
      //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") unknow name "<<name);
      bool exists = m_syncInterestTable.insert(digest, name.toUri(), true);
      if (exists) // somebody else replied, so restart random-game timer
        {
          m_scheduler.cancel(DELAYED_INTEREST_PROCESSING);
        }
      uint32_t waitDelay = m_rangeUniformRandom();
      //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") unknow name "<<name<<" wait time"<<waitDelay);
      m_scheduler.schedule(TIME_MILLISECONDS(200), bind(&RepoSyncRecovery::processSyncInterest, this, name, digest, true), DELAYED_INTEREST_PROCESSING);
    }
  else
    {
      m_syncInterestTable.remove(name.toUri());
      m_recoveryRetransmissionInterval = defaultRecoveryRetransmitInterval;
      sendRecoveryInterest(digest);
    }
}

void
RepoSyncRecovery::processFetchInterest(const Name& name)
{
  // if received fetch interest, the group is not synchronized, cancel the event of removeActions
  
  m_isSynchronized = false;
  m_scheduler.cancel(SYNCHRONIZED);
  Name actionName = name.getSubName(m_syncPrefix.size() + 1);
  //std::string seqNo = actionName.get(-1).to
  uint64_t seq = actionName.get(-1).toSeqNum();
  Name creator = actionName.getSubName(0, actionName.size() - 1);
  //std::cout<<m_creatorName<<" process Fetch interest name = "<<name<<std::endl;
  // check the sync tree to get the status of action's creator
  // if the requested action is removed, return the snapshot
  // Otherwise, send the action back
  SyncTree::const_iter iterator = m_syncTree.lookup(creator);
  //std::cout<<" before send snapshot creator = "<<creator<<" seq = "<<seq<<" first = "<<iterator->second.first<<std::endl;
  if (iterator != m_syncTree.end() && seq <= iterator->second.first && iterator->second.first != 0) {
    sendSnapshot(name);
    return;
  }
  std::list<std::pair<DigestPtr, ActionEntry> >::iterator it =
                                        std::find_if(m_actionList.begin(), m_actionList.end(),
                                                     bind(&compareActionEntry, _1, actionName));

  if (it != m_actionList.end()) {
    Msg message(SyncStateMsg::ACTION);
    message.writeActionToMsg(it->second);
    //m_scheduler.schedule(ns3::MilliSeconds(m_senddataJitter()), bind(&RepoSyncRecovery::sendData, this, name, message), 100);

    sendData(name, message);
  }
}

void
RepoSyncRecovery::processRecoveryInterest(const Name& name, DigestConstPtr digest)
{
  //std::cout<<"process recovery interest"<<std::endl;
  // if received recovery interest, the group is not synchronized, cancel the event of removeActions
  //if (GetNode()->GetId() == 11)
  DigestConstPtr rootDigest = m_syncTree.getDigest();
    std::ostringstream os;
  os << *m_syncTree.getDigest();
  
  m_outstandingInterestName.append("sync").append(os.str());
  //if (*rootDigest != *digest) 
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process recovery interest "<<name<<" local digest = "<<os.str());

  m_isSynchronized = false;
  m_scheduler.cancel(SYNCHRONIZED);
  // check into action list to see whether this digest has once appeared or not
  // if the digest can be recognized, send back the current status of all the known nodes
  // Otherwise, ignore this interest
  std::list<std::pair<DigestPtr, ActionEntry> >::iterator it = std::find_if(m_actionList.begin(),
                                                                            m_actionList.end(),
                                                                            bind(&compareDigest, _1, digest));
  if (it != m_actionList.end()) {
   //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process recovery interest "<<name);
    Msg message(SyncStateMsg::ACTION);
    SyncTree::const_iter iterator = m_syncTree.begin();
    while (iterator != m_syncTree.end()) {
      ActionEntry entry(iterator->first, iterator->second.last);
      //std::cout<<"writ message !!!!"<<std::endl;
      message.writeActionNameToMsg(entry);
      ++iterator;
    }
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process recovery interest "<<name<<" local digest = "<<os.str());
    sendData(name, message);
    //m_scheduler.schedule(ns3::MilliSeconds(m_senddataJitter()), bind(&RepoSyncRecovery::sendData, this, name, message), 100);
    checkInterestSatisfied(name);
  }
}

void
RepoSyncRecovery::sendSnapshot(const Name& name)
{
  //std::cout<<m_creatorName<<" send snapshot"<<std::endl;
  //NS_LOG_INFO ("***********************node("<< GetNode()->GetId() <<") send snapshot****************");
  sendData(name, m_snapshot);
  //m_scheduler.schedule(ns3::MilliSeconds(m_senddataJitter()), bind(&RepoSyncRecovery::sendData, this, name, m_snapshot), 100);
}

void
RepoSyncRecovery::writeDataToSnapshot(Msg* msg, const Name& name, const status& stat)
{
  //std::cout<<"snapshot name = "<<name<<std::endl;
  msg->writeDataToSnapshot(name, stat);
}

void
RepoSyncRecovery::sendSyncInterest()
{
  //std::cout<<m_creatorName<<"**************send sync interest**************  action size() =  "<<m_actionList.size()<<std::endl;
  //std::cout<<m_creatorName<<"interest digest is "<<*m_syncTree.getDigest()<<std::endl;
  
  m_outstandingInterestName = m_syncPrefix;
  std::ostringstream os;
  os << *m_syncTree.getDigest();
  
  m_outstandingInterestName.append("sync").append(os.str());
  Ptr<Interest> interest = Create<Interest>();
  interest->SetName(m_outstandingInterestName);
  interest->SetInterestLifetime(m_interestLifetime);
  ////NS_LOG_INFO ("node("<< GetNode()->GetId() <<") digest : " << *m_syncTree.getDigest());
  //if (GetNode()->GetId() == 0 || GetNode()->GetId() == 2)
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") interest name : " << interest->GetName()<<" action size = "<<m_actionList.size());

  m_ccnxHandle->sendInterest (m_outstandingInterestName.toUri (),
                              bind (&RepoSyncRecovery::onData, this, _1, _2, _3),
                              bind(&RepoSyncRecovery::onSyncTimeout, this, _1));
  m_scheduler.cancel(REEXPRESSING_INTEREST);
  m_scheduler.schedule(ns3::Seconds(syncInterestReexpress) + ns3::MilliSeconds(m_reexpressionJitter()),
                       bind (&RepoSyncRecovery::sendSyncInterest, this),
                       REEXPRESSING_INTEREST);
  
}

void
RepoSyncRecovery::sendFetchInterest(const Name& creatorName, const uint64_t& seq)
{
  Name actionName = creatorName;
  actionName.appendSeqNum(seq);
  //std::cout<<m_creatorName<<"send fetch interest name = "<<actionName<<" number = "<<m_retryTable[actionName]<<std::endl;
  // if the retry number of this fetch interest exceeds a certain value, stop fetching
  if (m_retryTable[actionName] >= retrytimes) {
    m_retryTable.erase(actionName);
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") cannot fetch action name = "<<creatorName<<" seq = "<<seq);
    throw Error("Cannot fetch the aciton");
  }
 /* char *num = new char[3];
  bzero(num, 3);
  sprintf(num,"%d", seq);*/

  Name interestName = m_syncPrefix;
  interestName.append("fetch").append(creatorName).appendSeqNum(seq);
  Ptr<Interest> interest = Create<Interest>();
  interest->SetName(interestName);
  interest->SetInterestLifetime(m_interestLifetime);
  //if (GetNode()->GetId() == 0 )
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") interest name : " << interest->GetName()<<" seq = "<<seq);
  m_ccnxHandle->sendInterest (interestName.toUri (),
                              bind (&RepoSyncRecovery::onData, this, _1, _2, _3),
                              bind(&RepoSyncRecovery::onFetchTimeout, this, _1));

  m_retryTable[actionName]++;
}

void
RepoSyncRecovery::onSyncTimeout(const std::string str)
{
  //std::cerr << "Sync interest timeout"<<std::endl;
  // It is OK. Others will handle the time out situation.
  ////NS_LOG_INFO ("node("<< GetNode()->GetId() <<") interest name : " << interest->GetName());
}


void
RepoSyncRecovery::onFetchTimeout(const std::string str)
{
  //std::cerr << "*********Fetch interest timeout***********" <<std::endl;

  Name name(str);

  Name actionName = name.getSubName(m_syncPrefix.size() + 1);
  uint64_t seq = actionName.get(-1).toSeqNum();
  Name creator = actionName.getSubName(0, actionName.size() - 1);
  //if (GetNode()->GetId() == 11)
    //NS_LOG_INFO ("!!!!!!!!!!!!!!!!!!!1node("<< GetNode()->GetId() <<") fetch interest timeout : " << str<<" count = "<<m_retryTable[actionName]);
  sendFetchInterest(creator, seq);
}

void
RepoSyncRecovery::onRecoveryTimeout(const std::string str)
{
  //std::cerr << "+++++++++++++++++++Recovery interest timeout+++++++++++++" <<std::endl;
  //NS_LOG_INFO ("+++++++++++++++++++node("<< GetNode()->GetId() <<") fetch interest timeout : " << str);
}

void
RepoSyncRecovery::setFilterTimeout(const std::string str)
{
  std::cerr << "-----------------register interest timeout---------------" <<std::endl;
}

void
RepoSyncRecovery::sendRecoveryInterest(DigestConstPtr digest)
{
  //std::cout<<"send recovery interest"<<std::endl;
  std::ostringstream os;
  os << *digest;

  Name interestName = m_syncPrefix;
  interestName.append("recovery").append(os.str());

  m_recoveryRetransmissionInterval <<= 1;

  m_scheduler.cancel(REEXPRESSING_RECOVERY_INTEREST);
  if (m_recoveryRetransmissionInterval < 100*1000) // <100 seconds
    m_scheduler.schedule(ns3::MilliSeconds(m_recoveryRetransmissionInterval + m_reexpressionJitter()),
                              bind(&RepoSyncRecovery::sendRecoveryInterest, this, digest), REEXPRESSING_RECOVERY_INTEREST);

  Ptr<Interest> interest= Create<Interest>();
  interest->SetName(interestName);
  interest->SetInterestLifetime(m_interestLifetime);
  //if (GetNode()->GetId() == 11)
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") !!!!!!!!!!!!!!!!!!send recovery interest name : " << interest->GetName());
  m_ccnxHandle->sendInterest (interestName.toUri (),
                              bind(&RepoSyncRecovery::onData, this, _1, _2, _3),
                              bind(&RepoSyncRecovery::onRecoveryTimeout, this, _1));
}

void
RepoSyncRecovery::checkInterestSatisfied(const Name &name)
{
  // checking if our own interest got satisfied
  // if satisfied schedule the event the resend the sync interest
  // std::cout<<"interest satisfied"<<std::endl;
  bool satisfiedOwnInterest = (m_outstandingInterestName == name);
  if (satisfiedOwnInterest)
    {
      // cout << "------------ reexpress interest after: " << after << endl;
      m_scheduler.cancel(REEXPRESSING_INTEREST);
      m_scheduler.schedule(ns3::MilliSeconds(0), bind(&RepoSyncRecovery::sendSyncInterest, this), REEXPRESSING_INTEREST);
    }
}

void
RepoSyncRecovery::sendData(const Name &name, Msg& ssm)
{
  
  int size = ssm.getMsg().ByteSize();
  char *wireData = new char[size];
  ssm.getMsg().SerializeToArray(wireData, size);

  Ptr<ndn::Data> data = Create<ndn::Data> (Create<Packet> (reinterpret_cast<const uint8_t*> (wireData), size));
  data->SetName(name);
  data->SetFreshness(ns3::Seconds(syncResponseFreshness));

  m_ccnxHandle->publishRawData (name.toUri(), wireData, size, 100); 
  //if (GetNode()->GetId() == 0)
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") data name : " << name<<" size = "<<size);
  /*
  ostringstream content;
  data->GetPayload()->CopyData(&content, data->GetPayload()->GetSize ());

  const char* wireData_test = content.str().c_str();
  size_t len = content.str().size();
  SyncStateMsg msg;
  std::cout<<"send data content "<<content.str()<<std::endl;
  if (!msg.ParseFromArray(wireData_test, len) || !msg.IsInitialized())
  {
    std::cout<<"!!!!!!!!!!!test size in send data "<<len<<std::endl;
    std::cout<<wireData_test<<std::endl;
    //Throw
    BOOST_THROW_EXCEPTION(Digest::SyncStateMsgDecodingFailure());
  }*/
  delete []wireData;
}


void
RepoSyncRecovery::onData(const std::string &str, const char *wireData, size_t len)
{
  Name name(str);
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<")  received data : " << name);

  try
    {
      std::string type = name[m_syncPrefix.size()].toUri();
      if (type == "sync")
        {
          DigestConstPtr digest = convertNameToDigest(name);
          m_syncInterestTable.remove(name.toUri());
          processSyncData(name, wireData, len);
        }
      else if (type == "fetch")
        {
          processFetchData(name, wireData, len);
        }
      else if (type == "recovery")
        {
          DigestConstPtr digest = convertNameToDigest(name);
          // timer is always restarted when we schedule recovery
          m_syncInterestTable.remove(name.toUri());
          m_scheduler.cancel(REEXPRESSING_RECOVERY_INTEREST);
          processRecoveryData(name, wireData, len);
        }
    }
  catch(ns3::ndn::Error::DigestCalculationError &e)
    {
      throw Error("Something fishy happened...");
      return;
    }
}

void
RepoSyncRecovery::processSyncData(const Name& name, const char* wireData, size_t len)
{
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process sync data "<<name);
  bool ownInterestSatisfied = false;
  ownInterestSatisfied = (name == m_outstandingInterestName);
  
  SyncStateMsg msg;
  //std::cout<<"process sync data = "<<name<<std::endl;
  if (!msg.ParseFromArray(wireData, len) || !msg.IsInitialized())
  {
    //Throw
    BOOST_THROW_EXCEPTION(Digest::SyncStateMsgDecodingFailure());
  }
  Msg message(msg);
  if (message.getMsg().type() == SyncStateMsg::ACTION) {
    message.readActionNameFromMsg(bind(&RepoSyncRecovery::prepareFetchForSync, this, _1, _2, _3), m_creatorName);
  }
  else {
    throw Error("The response of sync interest should not in this type!");
  }
  if (ownInterestSatisfied)
  {
    //system_clock::Duration after = milliseconds(m_reexpressionJitter());
    // std::cout << "------------ reexpress interest after: " << after << std::endl;
    m_scheduler.cancel(REEXPRESSING_INTEREST);
    m_scheduler.schedule(ns3::Seconds(4), bind(&RepoSyncRecovery::sendSyncInterest, this), REEXPRESSING_INTEREST); //wait more time for requesting actions

  }
}

void
RepoSyncRecovery::processFetchData(const Name& name, const char* wireData, size_t len)
{
  //if (GetNode()->GetId() == 11)
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process fetch data "<<name);
  SyncStateMsg msg;
  if (!msg.ParseFromArray(wireData, len) || !msg.IsInitialized())
  {
    BOOST_THROW_EXCEPTION(Digest::SyncStateMsgDecodingFailure() );
  }
  Msg message(msg);
  if (message.getMsg().type() == SyncStateMsg::ACTION) {
    // process action
    Name final = name.getSubName(0, name.size() - 1);
    final.appendSeqNum(50);
    //if (name == final)
      //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process ACTION "<<name);
    message.readActionFromMsg(bind(&RepoSyncRecovery::actionControl, this, _1));
  }
  else if (message.getMsg().type() == SyncStateMsg::SNAPSHOT) {
    // process snapshot
    
    std::pair<Name, uint64_t> info = message.readInfoFromSnapshot();

    std::list<std::pair<Name, uint64_t> >::iterator it =
                                        std::find_if(m_snapshotList.begin(), m_snapshotList.end(),
                                                     bind(&compareSnapshot, _1, info));
    // if snapshot has been fetched once, ignore it
    // Otherwise, process the snapshot and record it info
    if (it != m_snapshotList.end()) {
      return;
    }
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process SNAPSHOT "<<info.first);
    m_snapshotList.push_back(info);
    m_scheduler.schedule(ns3::Seconds(10),
                         bind(&RepoSyncRecovery::removeSnapshotEntry, this, m_snapshotList.back()), REMOVE_SNAPSHOT);

    message.readDataFromSnapshot(bind(&RepoSyncRecovery::processSnapshot, this, _1, _2));
    message.readTreeFromSnapshot(bind(&RepoSyncRecovery::updateSyncTree, this, _1));
  }
  else {
    throw Error("The response of fetch interest should not in this type!");
  }
}

void
RepoSyncRecovery::processRecoveryData(const Name& name, const char* wireData, size_t len)
{
  //if (GetNode()->GetId() == 1)
   //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") !!!!!!!!!!!!!!!!process recovery data "<<name);
  m_syncInterestTable.remove(name.toUri());
  //Msg message(SyncStateMsg::ACTION);
  SyncStateMsg msg;// = message.getMsg();
  if (!msg.ParseFromArray(wireData, len) || !msg.IsInitialized())
  {
    //Throw
    BOOST_THROW_EXCEPTION(Digest::SyncStateMsgDecodingFailure() );
  }
  Msg message(msg);
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") READ recovery data !!!!!!");
  message.readActionNameFromMsg(bind(&RepoSyncRecovery::prepareFetchForRecovery, this, _1, _2, _3), m_creatorName);
}

void
RepoSyncRecovery::prepareFetchForSync(const Name& name, const uint64_t seq, const uint64_t finalSeq)
{
  // this function is called when the sync interest digest is outdated
  // m_nodeSeq record the information of sequence number for each node
  // 'current' represents the last seq number the repo has
  // 'sending' represents the action seq number that is on fetching
  // 'final'   represents the last seq number that should be fetched
  //if (GetNode()->GetId() ==1 )
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") prepare for fetch seq "<<seq);
  SyncTree::const_iter iterator = m_syncTree.lookup(name);
  m_nodeSeq[name].final = finalSeq;
  uint64_t& sending = m_nodeSeq[name].sending;
  if (iterator != m_syncTree.end())
  {
    m_nodeSeq[name].current = iterator->second.last;
    if (iterator->second.last >= seq || sending >= seq) {
      //std::cerr << "Action has been fetched or sent" << std::endl;
      //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") Action has been fetched "<<name<<" seq = "<<seq);
      return;
    }
    if (sending == m_nodeSeq[name].current) {
       uint64_t lastSendSeq = (iterator->second.last+pipeline < seq ? iterator->second.last+pipeline : seq);
      for (uint64_t seqno = iterator->second.last + 1; seqno <= lastSendSeq; seqno++) {
          sendFetchInterest(name, seqno);
      }
      sending = lastSendSeq;
    }
  }
  else
  {
    m_nodeSeq[name].current = 0;
    m_syncTree.addNode(name);
    uint64_t lastSendSeq = (pipeline < finalSeq ? pipeline : finalSeq);
    for (uint64_t seqno = 1; seqno <= lastSendSeq; seqno++) {
      sendFetchInterest(name, seqno);
    }
    sending = lastSendSeq;
  }
}

void
RepoSyncRecovery::prepareFetchForRecovery(const Name& name, const uint64_t seq, const uint64_t finalSeq)
{
  //if (GetNode()->GetId() == 1)
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") prepare for recoevery "<<name<<" seq = "<<seq);
  // this function is called when the sync interest digest is unrecognized
  SyncTree::const_iter iterator = m_syncTree.lookup(name);
  m_nodeSeq[name].final = finalSeq;
  //std::cout<<"prepare fetch for recovery name ="<<name<<" seq = "<<m_nodeSeq[name].final<<std::endl;
  if (iterator != m_syncTree.end())
  {
    m_nodeSeq[name].current = iterator->second.last;
    if (iterator->second.last >= seq) {
      //std::cerr << "Action has been fetched" << std::endl;
      //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") Action has been fetched "<<name<<" seq = "<<seq);
      return;
    }
    if (m_nodeSeq[name].sending <= m_nodeSeq[name].current) {
      uint64_t lastSendSeq = (iterator->second.last+pipeline < finalSeq ? iterator->second.last+pipeline : finalSeq);
      for (uint64_t seqno = iterator->second.last + 1; seqno <= lastSendSeq; seqno++) {
        sendFetchInterest(name, seqno);
      }
      m_nodeSeq[name].sending = lastSendSeq;
    }
    else {
      std::list<ActionEntry>& pendingList = m_pendingActionList[name];
      if (!pendingList.empty())
      { 
        uint64_t pending = pendingList.begin()->getSeqNo();
        for (uint64_t seqno = m_nodeSeq[name].current + 1; seqno < pending; ++seqno)
        {
          Name creator = name;
          m_retryTable.erase(creator.appendSeqNum(seqno));
          sendFetchInterest(name, seqno);
        }
      }
    }
    
  }
  else
  {
    m_nodeSeq[name].current = 0;
    m_syncTree.addNode(name);
    uint64_t lastSendSeq = (pipeline < seq ? pipeline : seq);
    for (uint64_t seqno = 1; seqno <= lastSendSeq; seqno++) {
      sendFetchInterest(name, seqno);
    }
    m_nodeSeq[name].sending = lastSendSeq;
  }

}

void
RepoSyncRecovery::processSnapshot(const Name& name, const status& dataStatus)
{
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") process snapshot ");
  std::map<Name, status>::iterator it = m_storageHandle.find(name);
  status stat;
  if (it == m_storageHandle.end())
    stat = NONE;
  else
    stat = m_storageHandle[name];

  if (dataStatus == EXISTED) {
    if (stat == NONE) {  //if data is deleted, do not insert this data back
      sendNormalInterest(name);
    }
  }
  else if (dataStatus == DELETED) {
    //if data is inserted, means this data has once been deleted and inserted again,
    // so do not deleted the data.
    //we assume same data will not be deleted and inserted multiple times
    if (stat == EXISTED) {
      m_storageHandle[name] = DELETED;
    }
  }
  else {
    // if data status is INSERTED, update the deleted data
    if (stat == NONE || stat == DELETED) {
      sendNormalInterest(name);
    }
  }
}

void
RepoSyncRecovery::sendNormalInterest(const Name& name)
{
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") send normal interest : "<<name);
  Ptr<Interest> fetchInterest = Create<Interest>();
  fetchInterest->SetName(name);
  fetchInterest->SetInterestLifetime(m_interestLifetime);

   m_ccnxHandle->sendInterest (name.toUri(),
                              bind (&RepoSyncRecovery::onFetchData, this, _1, _2, _3),
                              bind(&RepoSyncRecovery::onDataTimeout, this, _1));
}

void
RepoSyncRecovery::actionControl(const ActionEntry& action)
{
  // use pipeline to control received action
  // if receives an action, send the action with seq number = min( received seq + pipeline, final )
  // if received action is in ordered, apply the action
  // Otherwise, save it in the pending table and retransmit all the missing actions
  m_reTransmit.erase(action.getName());
  uint64_t& currentSeq = m_nodeSeq[action.getCreatorName()].current;
  uint64_t& lastSeq = m_nodeSeq[action.getCreatorName()].final;
  uint64_t& sending = m_nodeSeq[action.getCreatorName()].sending;
  //if (GetNode()->GetId() == 11)
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") ACTION CONTROL "<<action.getSeqNo()<<" current = "<<currentSeq<< " lastSeq = "<<lastSeq<<" sending = "<<sending);
  if (action.getSeqNo() > lastSeq) {
    //std::cout<<"action name = "<<action.getCreatorName()<<" lastseq = "<<lastSeq<<std::endl;
    //throw Error("Received unrecognized sequence number ");
    //return;
    lastSeq = action.getSeqNo();
  }
  Name name = action.getCreatorName();
  std::list<ActionEntry>& pendingList = m_pendingActionList[name];
  m_retryTable.erase(action.getName());
  if (currentSeq + 1 == action.getSeqNo()) {
    currentSeq++;
    applyAction(action);
   
    while (!pendingList.empty() && pendingList.front().getSeqNo() == currentSeq + 1) {
      currentSeq++;
      applyAction(pendingList.front());
      pendingList.pop_front();
    }
     
    uint64_t seqno;
    if (currentSeq >= sending)
    {
      for (seqno = currentSeq + 1; seqno <= (currentSeq + pipeline <= lastSeq ? currentSeq + pipeline : lastSeq); seqno++) {  
        m_scheduler.schedule(ns3::MilliSeconds(0.01), bind(&RepoSyncRecovery::sendFetchInterest, this, name, seqno), 101);
      }
      sending = seqno - 1;
    }
    else
    {
      uint64_t step = currentSeq - action.getSeqNo();
      for (seqno = sending + 1; seqno <= (sending + 1 + step <= lastSeq ? sending + 1 + step : lastSeq); seqno++) {
        m_scheduler.schedule(ns3::MilliSeconds(0.01), bind(&RepoSyncRecovery::sendFetchInterest, this, name, seqno), 101);
      }
      sending = seqno - 1;
    }
    //if (GetNode()->GetId() == 11)
      //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") !!!!after "<<action.getSeqNo()<<" current = "<<currentSeq<< " lastSeq = "<<lastSeq<<" sending = "<<sending);
  }
  else if (currentSeq + 1 < action.getSeqNo()) {
    // retransmit
    std::list<ActionEntry>::iterator it = std::find_if(pendingList.begin(), pendingList.end(),
                                                                            bind(&compareSeq, _1, action.getSeqNo()));
    if (it != pendingList.end())
      return;
    pendingList.push_back(action);
    pendingList.sort();
    
    for (uint64_t seqno = currentSeq + 1; seqno < pendingList.begin()->getSeqNo(); ++seqno) {  // to be cchanged
      Name entry = action.getCreatorName();
      entry.appendSeqNum(seqno);
      if (m_reTransmit[entry] == 0) {
        m_reTransmit[entry]++;
        m_scheduler.schedule(ns3::MilliSeconds(0.01), bind(&RepoSyncRecovery::sendFetchInterest, this, action.getCreatorName(), seqno), 101);
        //sendFetchInterest(action.getCreatorName(), seqno);
        //if (GetNode()->GetId() == 11) 
          //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") retransmit seq = "<<seqno);
        
      }
      //

    }
    if (sending < pendingList.front().getSeqNo() - 1)
      sending = pendingList.front().getSeqNo() - 1;
  }
  else {
    // do nothing
  }
}

void
RepoSyncRecovery::applyAction(const ActionEntry& action)
{
  m_syncTree.update(action);
  // std::cout<<"update applyaction digest is = "<<m_syncTree.getDigest()<<std::endl;;
  //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") Apply action   !!!! ");
  m_actionList.push_back(std::make_pair(m_syncTree.getDigest(), action));
  if (action.getAction() == INSERTION) {
    Ptr<Interest> fetchInterest = Create<Interest>();
    fetchInterest->SetName(action.getDataName());
    fetchInterest->SetInterestLifetime(m_interestLifetime);
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") send normal interest : "<<action.getDataName());

      m_ccnxHandle->sendInterest (action.getDataName().toUri (),
                              bind (&RepoSyncRecovery::onFetchData, this, _1, _2, _3),
                              bind(&RepoSyncRecovery::onDataTimeout, this, _1));
  }
  else if (action.getAction() == DELETION) {
    std::map<Name, status>::iterator it = m_storageHandle.find(action.getDataName());
    if (it != m_storageHandle.end())
      m_storageHandle[action.getDataName()] = DELETED;
  }
  else {
    throw Error("Cannot apply this action type !");
  }
}

void
RepoSyncRecovery::onFetchData(const std::string &str, const char *wireData, size_t len)
{
  Name name(str);
  //Name final("data");
  //final.appendSeqNum(199);
  /*if (GetNode()->GetId() == 1)
    NS_LOG_INFO ("node("<< GetNode()->GetId() <<") receive normal data : "<<name<<"  stander data "<<final);
  if (name == final)
    NS_LOG_INFO ("node("<< GetNode()->GetId() <<") receive normal data : "<< name);*/
  std::map<Name, status>::iterator it = m_storageHandle.find(name);
  if (it == m_storageHandle.end())
    m_storageHandle[name] = EXISTED;
  else if (m_storageHandle[name] == DELETED) 
    m_storageHandle[name] = INSERTED;
}

void
RepoSyncRecovery::onDataTimeout(const std::string str)
{
  //std::cerr << "Fetch data timeout !" << std::endl;
  //if (GetNode()->GetId() == 11)
    //NS_LOG_INFO ("node("<< GetNode()->GetId() <<") data timeout : "<< str);
  m_ccnxHandle->sendInterest (str,
                              bind (&RepoSyncRecovery::onFetchData, this, _1, _2, _3),
                              bind(&RepoSyncRecovery::onDataTimeout, this, _1));
}

void
RepoSyncRecovery::processPendingSyncInterests()
{
  while (m_syncInterestTable.size() > 0)
  {
    InterestEntry interest = m_syncInterestTable.begin();
    processSyncInterest(interest.m_name, interest.m_digest, false);
    m_syncInterestTable.pop();
  }
}

void
RepoSyncRecovery::removeActions()
{
  init();
  m_retryTable.clear();
  m_pendingActionList.clear();
  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") CREATE SNAPSHOT!!!! ");
  createSnapshot();
}

void
RepoSyncRecovery::createSnapshot()
{
  //std::cout<<m_creatorName<<" createSnapshot seq = "<<m_snapshotNo<<""<<std::endl;
  
  Msg message(SyncStateMsg::SNAPSHOT);
  for (std::map<Name, status>::iterator it = m_storageHandle.begin(); it != m_storageHandle.end(); it++)
    writeDataToSnapshot(&message, it->first, it->second);
  for (SyncTree::const_iter iter = m_syncTree.begin(); iter != m_syncTree.end(); iter++) {
    message.writeTreeToSnapshot(iter->first, iter->second.last);
  }
  message.writeInfoToSnapshot(m_creatorName, m_snapshotNo);
  m_snapshot.setMsg(message.getMsg());
  m_snapshotNo++;
  m_syncTree.updateForSnapshot();
}

void
RepoSyncRecovery::updateSyncTree(const ActionEntry& entry)
{
  m_syncTree.update(entry);
  pipelineEntrySeq &node = m_nodeSeq[entry.getCreatorName()];
  node.current = entry.getSeqNo();
  node.sending = entry.getSeqNo();
  node.final = entry.getSeqNo() < node.final ? node.final : entry.getSeqNo();
}

void
RepoSyncRecovery::removeSnapshotEntry(std::pair<Name, uint64_t> info)
{
  m_snapshotList.remove(info);
}

void
RepoSyncRecovery::removeIndexEntry()
{
  for (std::map<Name, status>::iterator it = m_storageHandle.begin(); it != m_storageHandle.end();)
  {
    if (it->second == DELETED)
    {
      m_storageHandle.erase(it++);
    }
    else
      it++;
  }
  m_scheduler.schedule(ns3::Seconds(50), bind(&RepoSyncRecovery::removeIndexEntry, this), REMOVE_INDEX_ENTRY);
}


}
}

