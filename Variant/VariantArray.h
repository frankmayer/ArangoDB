////////////////////////////////////////////////////////////////////////////////
/// @brief variant class for arrays
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

#ifndef TRIAGENS_JUTLAND_BASICS_VARIANT_ARRAY_H
#define TRIAGENS_JUTLAND_BASICS_VARIANT_ARRAY_H 1

#include <Variant/VariantObject.h>

namespace triagens {
  namespace basics {
    class VariantBlob;
    class VariantBoolean;
    class VariantDate;
    class VariantDatetime;
    class VariantDouble;
    class VariantFloat;
    class VariantInt8;
    class VariantInt16;
    class VariantInt32;
    class VariantInt64;
    class VariantMatrix2;
    class VariantNull;
    class VariantString;
    class VariantUInt8;
    class VariantUInt16;
    class VariantUInt32;
    class VariantUInt64;
    class VariantVector;

    ////////////////////////////////////////////////////////////////////////////////
    /// @ingroup Variants
    /// @brief variant class for arrays
    ////////////////////////////////////////////////////////////////////////////////

    class VariantArray : public VariantObject {
      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief type of VariantObject
        ////////////////////////////////////////////////////////////////////////////////

        static ObjectType const TYPE = VARIANT_ARRAY;

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new array
        ////////////////////////////////////////////////////////////////////////////////

        VariantArray ();

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief constructs a new array
        ////////////////////////////////////////////////////////////////////////////////

        ~VariantArray ();

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        ObjectType type () const {
          return TYPE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        VariantObject* clone () const;

        ////////////////////////////////////////////////////////////////////////////////
        /// {@inheritDoc}
        ////////////////////////////////////////////////////////////////////////////////

        void print (StringBuffer&, size_t indent) const;

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief returns the attributes
        ////////////////////////////////////////////////////////////////////////////////

        vector<string> const& getAttributes () const {
          return attributes;
        }

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief returns the values
        ////////////////////////////////////////////////////////////////////////////////

        vector<VariantObject*> getValues () const;

      public:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief adds a key value pair
        ////////////////////////////////////////////////////////////////////////////////

        void add (string const& key, VariantObject* value);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief adds a key value pair
        ////////////////////////////////////////////////////////////////////////////////

        void add (string const& key, string const& value);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief adds many key value pairs
        ////////////////////////////////////////////////////////////////////////////////

        void add (vector <string> const& kv);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief prepares next key/value pair
        ////////////////////////////////////////////////////////////////////////////////

        void addNextKey (string const& key);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief finishes next key/value pair
        ////////////////////////////////////////////////////////////////////////////////

        void addNextValue (VariantObject* value);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up an attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantObject* lookup (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up an array attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantArray* lookupArray (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a blob attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantBlob* lookupBlob (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a boolean attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantBoolean* lookupBoolean (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a date attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantDate* lookupDate (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a datetime attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantDatetime* lookupDatetime (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a double attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantDouble* lookupDouble (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a float attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantFloat* lookupFloat (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a int16 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantInt8* lookupInt8 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a int16 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantInt16* lookupInt16 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a int32 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantInt32* lookupInt32 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a int64 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantInt64* lookupInt64 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a matrix attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantMatrix2* lookupMatrix2 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a null attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantNull* lookupNull (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a string attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantString* lookupString (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a string attribute
        ////////////////////////////////////////////////////////////////////////////////

        string const& lookupString (string const& name, bool& found);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a uint8 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantUInt8* lookupUInt8 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a uint16 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantUInt16* lookupUInt16 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a uint32 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantUInt32* lookupUInt32 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a uint64 attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantUInt64* lookupUInt64 (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a vector attribute
        ////////////////////////////////////////////////////////////////////////////////

        VariantVector* lookupVector (string const& name);

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief looks up a vector attribute
        ///
        /// The attribute must be a vector of VariantString. If there is an entry which
        /// is not a VariantString or the attribute is not a VariantVector than an
        /// error is flagged.
        ////////////////////////////////////////////////////////////////////////////////

        vector<string> lookupStrings (string const& name, bool& error);

      protected:

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief attributes names
        ////////////////////////////////////////////////////////////////////////////////

        vector<string> attributes;

        ////////////////////////////////////////////////////////////////////////////////
        /// @brief values
        ////////////////////////////////////////////////////////////////////////////////

        map<string, VariantObject*> mapping;

      private:
        string nextKey;
    };
  }
}

#endif
