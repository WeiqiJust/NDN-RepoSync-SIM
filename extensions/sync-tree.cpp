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


#include "sync-tree.hpp"

namespace ns3 {
namespace ndn {

DigestPtr
SyncTree::update(const ActionEntry& action)
{
  Name creator = action.getCreatorName();
  std::map<Name, TreeEntry>::iterator it = m_nodes.find(creator);
  if (it == m_nodes.end()) {
    TreeEntry entry;
    entry.first = 0;
    entry.last = action.getSeqNo();
    entry.digest = action.getDigest();
    m_nodes[creator] = entry;
  }
  else {
    if (it->second.last < action.getSeqNo()) {
      //it->second.first = action.getSeqNo();
      it->second.last = action.getSeqNo();
      it->second.digest = action.getDigest();
    }
    else {
      // do nothing, this situation can only happen when fetching actions responses are out of order
    }
  }
  return calculateDigest();
}

void
SyncTree::updateForSnapshot()
{
  for (std::map<Name, TreeEntry>::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
  {
    it->second.first = it->second.last;
  }
}

void
SyncTree::addNode(const Name& name)
{
  TreeEntry entry;
  entry.first = 0;
  entry.last = 0;
  entry.digest = make_shared<Digest>();
  *entry.digest << name.toUri() << entry.last;
  entry.digest->finalize();
  m_nodes[name] = entry;
}

DigestPtr
SyncTree::calculateDigest()
{
  std::map<Name, TreeEntry>::iterator it = m_nodes.begin();
  shared_ptr<Digest> digest = make_shared<Digest>();
  while (it != m_nodes.end()) {
    *digest << *it->second.digest;
    ++it;
  }
  digest->finalize();
  m_root = digest;
  return m_root;
}

SyncTree::const_iter
SyncTree::lookup(const Name& creatorName) const
{
  return m_nodes.find(creatorName);
}

}
}
