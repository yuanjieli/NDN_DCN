/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
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
 * Author: Yuanjie Li <yuanjie.li@cs.ucla.edu>
 */

#ifndef _NDN_LIMITS_DELTA_RATE_H_
#define	_NDN_LIMITS_DELTA_RATE_H_

#include "ndn-limits.h"
#include <ns3/nstime.h>

namespace ns3 {
namespace ndn {

/**
 * \ingroup ndn
 * \brief Structure to manage limits for outstanding interests
 */
class LimitsDeltaRate :
    public Limits
{
public:
  typedef Limits super;

  static TypeId
  GetTypeId ();

  /**
   * \brief Constructor
   * \param prefix smart pointer to the prefix for the FIB entry
   */
  LimitsDeltaRate ();
    

  virtual
  ~LimitsDeltaRate () { }

	/**
   * \brief Set Interest limit
   * \param rate Interest limit (packet/s)
   * \param delay not used here. The rate here is independent of delay/RTT
   */
  virtual void
  SetLimits (double rate, double delay);

  virtual
  double
  GetMaxLimit () const
  {
    return GetMaxRate ();
  }
  
  virtual
  double GetAvailableInterestIncrement () const;

  /**
   * @brief Check if Interest limit is reached (token bucket is not empty)
   */
  virtual bool
  IsBelowLimit ();

  /**
   * @brief Get token from the bucket
   */
  virtual void
  BorrowLimit ();

  /**
   * @brief Does nothing (token bucket leakage is time-dependent only)
   */
  virtual void
  ReturnLimit ();

  /**
   * @brief Update normalized amount that should be leaked every second (token bucket leak rate) and leak rate
   */
  virtual void
  UpdateCurrentLimit (double limit);

  virtual double
  GetCurrentLimit () const
  {
  	return m_bucketMax;
  }
  
  virtual double
  GetCurrentLimitRate () const
  {
    return m_bucketMax;
  }
  
  virtual double
  GetCurrentCounter () const
  {
  	return m_bucket;
  }
  
  virtual double
  GetNack() const
  {
  	return m_nack;
  }
  
  virtual void
  IncreaseNack()
  {
  	m_nack++;
  }

protected:
  // from Node
  void
  NotifyNewAggregate ();

private:
  
  /**
   * @brief Reset Interest counter, and update positive Interest arrival rate variation
   */
  void
  UpdateBucket ();
  
private:
  bool m_isLeakScheduled;

  double m_bucketMax;   ///< \brief Maximum Interest allowance for this face (packet/s)
  double m_bucket;			///< \brief Interest packet counter. will be reset every m_resetInterval. Equivalent to actual Interest arrival rate
  double m_bucketOld;		///< \brief Interest packet counter in last round. Used for updating m_Deltabucket
  double m_resetInterval;	///< \brief Every m_resetInterval the packet counter will be reset, and m_Deltabucket will be updated
  double m_nack;				///< \brief number of NACKs received from this face
  //double m_oldnack;
  
};


} // namespace ndn
} // namespace ns3

#endif // _NDN_LIMITS_DELTA_RATE_H_
