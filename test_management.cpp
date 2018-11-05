/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2018 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Alexander Afanasyev <http://lasr.cs.ucla.edu/afanasyev/index.html>
 */

// correct way to include ndn-cxx headers
// #include <ndn-cxx/face.hpp>
// #include <ndn-cxx/security/key-chain.hpp>
#include "face.hpp"
#include "security/key-chain.hpp"

#include <iostream>

// Enclosing code in ndn simplifies coding (can also use `using namespace ndn`)
namespace ndn {
// Additional nested namespaces can be used to prevent/limit name conflicts
namespace examples {

class Mgmt : noncopyable
{
public:
  void
  run()
  {
    m_face.setInterestFilter("/Simulator/BATT",
                             bind(&Mgmt::onInterest, this, _1, _2),
                             RegisterPrefixSuccessCallback(),
                             bind(&Mgmt::onRegisterFailed, this, _1, _2));

    m_face.processEvents();
  }

private:
  void
  onInterest(const InterestFilter& filter, const Interest& interest)
  {
    std::cout << "<< M_I: " << interest << std::endl;

    if (true){
      ForwardingInterest();
      bind(&Mgmt::createData, this, _1);
    }

    else 
      bind(&Mgmt::forwardingNack, this, _1, _2);
  }

  void
  onRegisterFailed(const Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
  }

  void
  ForwardingInterest()
  {
     Interest _interest(Name("/Cloud/Simulator/DA1549"));
    _interest.setInterestLifetime(10_s);
    _interest.setMustBeFresh(true);
    
    m_face.expressInterest(_interest,
                          bind(&Mgmt::AckData, this, _1, _2),
                          bind(&Mgmt::forwardingNack, this, _1, _2),
                          bind(&Mgmt::forwardingTimeout, this, _1));
  
    std::cout << "Forwarding: " << _interest << std::endl;

    m_face.processEvents();
  }

  void
  AckData(const Interest& interest, const Data& data)
  {
    std::cout << "M_D: " << data << std::endl;
  }

  void
  forwardingNack(const Interest& interest, const lp::Nack& nack)
  {
    std::cout << "received Nack with reason ? " << nack.getReason()
              << " for interest " << interest << std::endl;
  }

  void
  forwardingTimeout(const Interest& interest)
  {
    std::cout << "Timeout " << interest << std::endl;
  }

  void
  createData(const Interest& interest)
  {
    Name dataNames(interest.getName());
    dataNames
            .append("BATT")
            .appendVersion();
    static const std::string contents = "BATT Simulation";

    shared_ptr<Data> data = make_shared<Data>();
    data->setName(dataNames);
    data->setFreshnessPeriod(20_s);
    data->setContent(reinterpret_cast<const uint8_t*>(contents.data()), contents.size());

    m_keyChain.sign(*data);

    std::cout << ">>D: " << *data << std::endl;

    m_face.put(*data);
  }

private:
  Face m_face;
  KeyChain m_keyChain;
};

} // namespace examples
} // namespace ndn

int
main(int argc, char** argv)
{
  ndn::examples::Mgmt mgmt;
  try {
    mgmt.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
