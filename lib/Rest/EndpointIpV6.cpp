////////////////////////////////////////////////////////////////////////////////
/// @brief connection endpoint, ipv6-based
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2013 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Copyright 2012-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "EndpointIpV6.h"

#include "Rest/Endpoint.h"

using namespace triagens::basics;
using namespace triagens::rest;

// -----------------------------------------------------------------------------
// --SECTION--                                                      EndpointIpV6
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                        constructors / destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Rest
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief creates an IPv6 socket endpoint
////////////////////////////////////////////////////////////////////////////////

EndpointIpV6::EndpointIpV6 (const Endpoint::EndpointType type,
                            const Endpoint::EncryptionType encryption,
                            const std::string& specification,
                            int listenBacklog,
                            bool reuseAddress,
                            const std::string& host,
                            const uint16_t port) 
  : EndpointIp(type, DOMAIN_IPV6, encryption, specification, listenBacklog, reuseAddress, host, port) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief destroys an IPv6 socket endpoint
////////////////////////////////////////////////////////////////////////////////

EndpointIpV6::~EndpointIpV6 () {
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
