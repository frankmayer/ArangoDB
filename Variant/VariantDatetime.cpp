////////////////////////////////////////////////////////////////////////////////
/// @brief class for result datetimes
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2011 triagens GmbH, Cologne, Germany
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
/// @author Dr. Frank Celler
/// @author Achim Brandt
/// @author Copyright 2008-2011, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "VariantDatetime.h"

#include <Basics/StringBuffer.h>

namespace triagens {
  namespace basics {

    // -----------------------------------------------------------------------------
    // constructors and destructors
    // -----------------------------------------------------------------------------

    VariantDatetime::VariantDatetime (datetime_t value)
      : VariantObjectTemplate<datetime_t, VariantObject::VARIANT_DATETIME, VariantDatetime>(value) {
    }

    // -----------------------------------------------------------------------------
    // VariantObject methods
    // -----------------------------------------------------------------------------

    void VariantDatetime::print (StringBuffer& buffer, size_t) const {
      buffer.appendText("(datetime) ");
      buffer.appendDecimal(getValue());
      buffer.appendEol();
    }
  }
}
