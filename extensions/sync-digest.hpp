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


#ifndef REPO_SYNC_SYNC_DIGEST_H
#define REPO_SYNC_SYNC_DIGEST_H

#include "common.hpp"

#include <vector>

#include <boost/cstdint.hpp>
#include <boost/exception/all.hpp>
#include <openssl/evp.h>


namespace ns3 {
namespace ndn {

/**
 * @ingroup sync
 * @brief A simple wrapper for libcrypto hash functions
 */
class Digest
{
public:
  /**
   * @brief Will be thrown when data cannot be properly decoded to SyncStateMsg
   */
  struct SyncStateMsgDecodingFailure : virtual boost::exception, virtual std::exception{ };

public:
  /**
   * @brief Default constructor.  Will initialize internal libssl structures
   */
  Digest();

  /**
   * @brief Check if digest is empty
   */
  bool
  empty() const;

  /**
   * @brief Reset digest to the initial state
   */
  void
  reset();

  /**
   * @brief Destructor
   */
  ~Digest();

  /**
   * @brief Obtain a short version of the hash (just first sizeof(size_t) bytes
   *
   * Side effect: finalize() will be called on `this'
   */
  std::size_t
  getHash() const;

  /**
   * @brief Finalize digest. All subsequent calls to "operator <<" will fire an exception
   */
  void
  finalize();

  /**
   * @brief Compare two full digests
   *
   * Side effect: Finalize will be called on `this' and `digest'
   */
  bool
  operator == (const Digest &digest) const;

  bool
  operator != (const Digest &digest) const
  { return !(*this == digest); }


  /**
   * @brief Add existing digest to digest calculation
   * @param src digest to combine with
   *
   * The result of this combination is  hash (hash (...))
   */
  Digest &
  operator << (const Digest &src);

  /**
   * @brief Add string to digest calculation
   * @param str string to put into digest
   */
  inline Digest &
  operator << (const std::string &str);

  /**
   * @brief Add uint64_t value to digest calculation
   * @param value uint64_t value to put into digest
   */
  Digest &
  operator << (uint64_t value);

  /**
   * @brief Checks if the stored hash is zero-root hash
   *
   * Zero-root hash is a valid hash that optimally represents an empty state
   */
  bool
  isZero() const;

private:
  Digest &
  operator = (Digest &digest)
  {
    (void)digest;
    return *this;
  }

  /**
   * @brief Add size bytes of buffer to the hash
   */
  void
  update(const uint8_t *buffer, size_t size);

  friend std::ostream &
  operator << (std::ostream &os, const Digest &digest);

  friend std::istream &
  operator >> (std::istream &is, Digest &digest);

private:
  EVP_MD_CTX *m_context;
  std::vector<uint8_t> m_buffer;
};

namespace Error {
struct DigestCalculationError : virtual boost::exception, virtual std::exception{ };
}

typedef boost::shared_ptr<Digest> DigestPtr;
typedef boost::shared_ptr<const Digest> DigestConstPtr;

Digest &
Digest::operator << (const std::string &str)
{
  update (reinterpret_cast<const uint8_t*> (str.c_str()), str.size());
  return *this;
}

std::ostream &
operator << (std::ostream &os, const Digest &digest);

std::istream &
operator >> (std::istream &is, Digest &digest);

struct DigestPtrHash : public std::unary_function<Digest, std::size_t>
{
  std::size_t
  operator() (DigestConstPtr digest) const
  {
    // std::cout << "digest->getHash: " << digest->getHash () << " (" << *digest << ")" << std::endl;
    return digest->getHash();
  }
};

struct DigestPtrEqual : public std::unary_function<Digest, std::size_t>
{
  bool
  operator() (DigestConstPtr digest1, DigestConstPtr digest2) const
  {
    // std::cout << boost::cref(*digest1) << " == " << boost::cref(*digest2)
    // << " : " << (*digest1 == *digest2) << std::endl;
    return *digest1 == *digest2;
  }
};

}
}

#endif // REPO_SYNC_SYNC_DIGEST_H
