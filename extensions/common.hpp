/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California.
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
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * repo-ng, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REPO_COMMON_HPP
#define REPO_COMMON_HPP

#include <boost/utility.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/geometric_distribution.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/random.hpp>
#include <boost/function.hpp>
#include <boost/exception/all.hpp>
#include <boost/make_shared.hpp>

#include <ns3/ptr.h>
#include <ns3/node.h>
#include <ns3/random-variable.h>
#include <ns3/ndn-app.h>
#include <ns3/ndn-name.h>
#include <ns3/ndn-data.h>
#include <ns3/ndn-interest.h>
#include <ns3/ndn-common.h>
#include <ns3/string.h>
#include <ns3/nstime.h>
#include "ns3/ndnSIM/ndn.cxx/ndn-api-face.h"
#include <map>
#include <string>
#include <vector>
#include <queue>
#include <list>
#include <algorithm>
#include <iostream>

namespace ns3 {
namespace ndn {

using namespace ns3;
using namespace boost;
using namespace std;

using ns3::ndn::Name ;
using ns3::ndn::Interest;
using ns3::ndn::Data;
using ns3::ndn::ApiFace;

//using boost::make_shared;
using std::vector;
using std::string;

using boost::noncopyable;

typedef uint64_t ProcessId;
typedef uint64_t SegmentNo;

}
}

#endif // REPO_COMMON_HPP
