////////////////////////////////////////////////////////////////////////////////
/// @brief V8 utility functions
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
/// @author Dr. Frank Celler
/// @author Copyright 2011-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "v8-conv.h"

#include "Basics/StringUtils.h"
#include "BasicsC/conversions.h"
#include "BasicsC/logging.h"
#include "BasicsC/string-buffer.h"
#include "BasicsC/tri-strings.h"
#include "ShapedJson/shaped-json.h"

#include "V8/v8-json.h"
#include "V8/v8-utils.h"

using namespace std;
using namespace triagens::basics;

// #define DEBUG_JSON_SHAPER 1

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

static int FillShapeValueJson (TRI_shaper_t* shaper,
                               TRI_shape_value_t* dst,
                               v8::Handle<v8::Value> const& json,
                               set<int>& seenHashes,
                               vector< v8::Handle<v8::Object> >& seenObjects,
                               bool create,
                               bool isLocked);

static v8::Handle<v8::Value> JsonShapeData (TRI_shaper_t* shaper,
                                            TRI_shape_t const* shape,
                                            char const* data,
                                            size_t size);

// -----------------------------------------------------------------------------
// --SECTION--                                                     private types
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief shape cache (caches pointer to last shape)
////////////////////////////////////////////////////////////////////////////////

typedef struct shape_cache_s {
  TRI_shape_sid_t    _sid;
  TRI_shape_t const* _shape;
}
shape_cache_t;

// -----------------------------------------------------------------------------
// --SECTION--                                              CONVERSION FUNCTIONS
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                 private functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a null into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueNull (TRI_shaper_t* shaper, 
                                TRI_shape_value_t* dst) {
  dst->_type = TRI_SHAPE_NULL;
  dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_NULL);
  dst->_fixedSized = true;
  dst->_size = 0;
  dst->_value = 0;

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a boolean into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueBoolean (TRI_shaper_t* shaper,  
                                  TRI_shape_value_t* dst, 
                                  v8::Handle<v8::Boolean> const& json) {
  TRI_shape_boolean_t* ptr;

  dst->_type = TRI_SHAPE_BOOLEAN;
  dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_BOOLEAN);
  dst->_fixedSized = true;
  dst->_size = sizeof(TRI_shape_boolean_t);
  dst->_value = (char*) (ptr = (TRI_shape_boolean_t*) TRI_Allocate(shaper->_memoryZone, dst->_size, false));

  if (dst->_value == 0) {
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  *ptr = json->Value() ? 1 : 0;

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a boolean into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueBoolean (TRI_shaper_t* shaper, 
                                  TRI_shape_value_t* dst, 
                                  v8::Handle<v8::BooleanObject> const& json) {
  TRI_shape_boolean_t* ptr;

  dst->_type = TRI_SHAPE_BOOLEAN;
  dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_BOOLEAN);
  dst->_fixedSized = true;
  dst->_size = sizeof(TRI_shape_boolean_t);
  dst->_value = (char*) (ptr = (TRI_shape_boolean_t*) TRI_Allocate(shaper->_memoryZone, dst->_size, false));

  if (dst->_value == 0) {
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  *ptr = json->BooleanValue() ? 1 : 0;

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a number into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueNumber (TRI_shaper_t* shaper, 
                                 TRI_shape_value_t* dst, 
                                 v8::Handle<v8::Number> const& json) {
  TRI_shape_number_t* ptr;

  dst->_type = TRI_SHAPE_NUMBER;
  dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_NUMBER);
  dst->_fixedSized = true;
  dst->_size = sizeof(TRI_shape_number_t);
  dst->_value = (char*) (ptr = (TRI_shape_number_t*) TRI_Allocate(shaper->_memoryZone, dst->_size, false));

  if (dst->_value == 0) {
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  *ptr = json->Value();

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a number into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueNumber (TRI_shaper_t* shaper, 
                                 TRI_shape_value_t* dst, 
                                 v8::Handle<v8::NumberObject> const& json) {
  TRI_shape_number_t* ptr;

  dst->_type = TRI_SHAPE_NUMBER;
  dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_NUMBER);
  dst->_fixedSized = true;
  dst->_size = sizeof(TRI_shape_number_t);
  dst->_value = (char*) (ptr = (TRI_shape_number_t*) TRI_Allocate(shaper->_memoryZone, dst->_size, false));

  if (dst->_value == 0) {
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  *ptr = json->NumberValue();

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a string into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueString (TRI_shaper_t* shaper, 
                                 TRI_shape_value_t* dst, 
                                 v8::Handle<v8::String> const& json) {
  char* ptr;

  TRI_Utf8ValueNFC str(TRI_UNKNOWN_MEM_ZONE, json);

  if (*str == 0) {
    dst->_type = TRI_SHAPE_SHORT_STRING;
    dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_SHORT_STRING);
    dst->_fixedSized = true;
    dst->_size = sizeof(TRI_shape_length_short_string_t) + TRI_SHAPE_SHORT_STRING_CUT;
    dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, true));

    if (dst->_value == 0) {
      return TRI_ERROR_OUT_OF_MEMORY;
    }

    * ((TRI_shape_length_short_string_t*) ptr) = 1;
    * (ptr + sizeof(TRI_shape_length_short_string_t)) = '\0';
  }
  else {
    const size_t size = str.length();

    if (size < TRI_SHAPE_SHORT_STRING_CUT) { // includes '\0'
      dst->_type = TRI_SHAPE_SHORT_STRING;
      dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_SHORT_STRING);
      dst->_fixedSized = true;
      dst->_size = sizeof(TRI_shape_length_short_string_t) + TRI_SHAPE_SHORT_STRING_CUT;
      dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, true));

      if (dst->_value == 0) {
        return TRI_ERROR_OUT_OF_MEMORY;
      }

      * ((TRI_shape_length_short_string_t*) ptr) = size + 1;
      memcpy(ptr + sizeof(TRI_shape_length_short_string_t), *str, size + 1);
    }
    else {
      dst->_type = TRI_SHAPE_LONG_STRING;
      dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_LONG_STRING);
      dst->_fixedSized = false;
      dst->_size = sizeof(TRI_shape_length_long_string_t) + size + 1;
      dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, true));

      if (dst->_value == 0) {
        return TRI_ERROR_OUT_OF_MEMORY;
      }

      * ((TRI_shape_length_long_string_t*) ptr) = size + 1;
      memcpy(ptr + sizeof(TRI_shape_length_long_string_t), *str, size + 1);
    }
  }

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a json list into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueList (TRI_shaper_t* shaper,
                               TRI_shape_value_t* dst,
                               v8::Handle<v8::Array> const& json,
                               set<int>& seenHashes,
                               vector< v8::Handle<v8::Object> >& seenObjects,
                               bool create,
                               bool isLocked) {
  size_t i, n;
  size_t total;

  TRI_shape_value_t* values;
  TRI_shape_value_t* p;
  TRI_shape_value_t* e;

  bool hs;
  bool hl;

  TRI_shape_sid_t s;
  TRI_shape_sid_t l;

  TRI_shape_sid_t* sids;
  TRI_shape_size_t* offsets;
  TRI_shape_size_t offset;

  TRI_shape_t const* found;

  char* ptr;

  // check for special case "empty list"
  n = json->Length();

  if (n == 0) {
    dst->_type = TRI_SHAPE_LIST;
    dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_LIST);

    dst->_fixedSized = false;
    dst->_size = sizeof(TRI_shape_length_list_t);
    dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, false));

    if (dst->_value == 0) {
      return TRI_ERROR_OUT_OF_MEMORY;
    }

    * (TRI_shape_length_list_t*) ptr = 0;

    return TRI_ERROR_NO_ERROR;
  }

  // convert into TRI_shape_value_t array
  p = (values = (TRI_shape_value_t*) TRI_Allocate(shaper->_memoryZone, sizeof(TRI_shape_value_t) * n, true));

  if (p == 0) {
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  total = 0;
  e = values + n;

  for (i = 0;  i < n;  ++i, ++p) {
    v8::Handle<v8::Value> el = json->Get(i);
    int res = FillShapeValueJson(shaper, p, el, seenHashes, seenObjects, create, isLocked);

    if (res != TRI_ERROR_NO_ERROR) {
      for (e = p, p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);
      return res;
    }

    total += p->_size;
  }

  // check if this list is homoegenous
  hs = true;
  hl = true;

  s = values[0]._sid;
  l = values[0]._size;
  p = values;

  for (;  p < e;  ++p) {
    if (p->_sid != s) {
      hs = false;
      break;
    }

    if (p->_size != l) {
      hl = false;
    }
  }

  // homogeneous sized
  if (hs && hl) {
    TRI_homogeneous_sized_list_shape_t* shape;

    shape = (TRI_homogeneous_sized_list_shape_t*) TRI_Allocate(shaper->_memoryZone, sizeof(TRI_homogeneous_sized_list_shape_t), true);

    if (shape == 0) {
      for (p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);
      return TRI_ERROR_OUT_OF_MEMORY;
    }

    shape->base._size = sizeof(TRI_homogeneous_sized_list_shape_t);
    shape->base._type = TRI_SHAPE_HOMOGENEOUS_SIZED_LIST;
    shape->base._dataSize = TRI_SHAPE_SIZE_VARIABLE;
    shape->_sidEntry = s;
    shape->_sizeEntry = l;

    found = shaper->findShape(shaper, &shape->base, create, isLocked);

    if (found == 0) {
      for (p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);
      TRI_Free(shaper->_memoryZone, shape);

      LOG_TRACE("shaper failed to find shape of type %d", (int) shape->base._type);

      if (! create) {
        return TRI_RESULT_ELEMENT_NOT_FOUND;
      }

      return TRI_ERROR_INTERNAL;
    }

    assert(found != 0);

    dst->_type = found->_type;
    dst->_sid = found->_sid;

    dst->_fixedSized = false;
    dst->_size = sizeof(TRI_shape_length_list_t) + total;
    dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, false));

    if (dst->_value == NULL) {
      for (p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);

      return TRI_ERROR_OUT_OF_MEMORY;
    }

    // copy sub-objects into data space
    * (TRI_shape_length_list_t*) ptr = n;
    ptr += sizeof(TRI_shape_length_list_t);

    for (p = values;  p < e;  ++p) {
      memcpy(ptr, p->_value, p->_size);
      ptr += p->_size;
    }
  }

  // homogeneous
  else if (hs) {
    TRI_homogeneous_list_shape_t* shape;

    shape = (TRI_homogeneous_list_shape_t*) TRI_Allocate(shaper->_memoryZone, sizeof(TRI_homogeneous_list_shape_t), true);

    if (shape == NULL) {
      for (p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);

      return TRI_ERROR_OUT_OF_MEMORY;
    }

    shape->base._size = sizeof(TRI_homogeneous_list_shape_t);
    shape->base._type = TRI_SHAPE_HOMOGENEOUS_LIST;
    shape->base._dataSize = TRI_SHAPE_SIZE_VARIABLE;
    shape->_sidEntry = s;

    // if found returns non-NULL, it will free the shape!!
    found = shaper->findShape(shaper, &shape->base, create, isLocked);

    if (found == 0) {
      for (p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }
      
      LOG_TRACE("shaper failed to find shape %d", (int) shape->base._type);

      TRI_Free(shaper->_memoryZone, values);
      TRI_Free(shaper->_memoryZone, shape);

      if (! create) {
        return TRI_RESULT_ELEMENT_NOT_FOUND;
      }

      return TRI_ERROR_INTERNAL;
    }

    assert(found != 0);

    dst->_type = found->_type;
    dst->_sid = found->_sid;

    offset = sizeof(TRI_shape_length_list_t) + (n + 1) * sizeof(TRI_shape_size_t);

    dst->_fixedSized = false;
    dst->_size = offset + total;
    dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, true));

    if (dst->_value == 0) {
      for (p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);

      return TRI_ERROR_OUT_OF_MEMORY;
    }

    // copy sub-objects into data space
    * (TRI_shape_length_list_t*) ptr = n;
    ptr += sizeof(TRI_shape_length_list_t);

    offsets = (TRI_shape_size_t*) ptr;
    ptr += (n + 1) * sizeof(TRI_shape_size_t);

    for (p = values;  p < e;  ++p) {
      *offsets++ = offset;
      offset += p->_size;

      memcpy(ptr, p->_value, p->_size);
      ptr += p->_size;
    }

    *offsets = offset;
  }

  // in-homogeneous
  else {
    dst->_type = TRI_SHAPE_LIST;
    dst->_sid = TRI_LookupBasicSidShaper(TRI_SHAPE_LIST);

    offset =
      sizeof(TRI_shape_length_list_t)
      + n * sizeof(TRI_shape_sid_t)
      + (n + 1) * sizeof(TRI_shape_size_t);

    dst->_fixedSized = false;
    dst->_size = offset + total;
    dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, true));

    if (dst->_value == NULL) {
      for (p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);
      return TRI_ERROR_OUT_OF_MEMORY;
    }

    // copy sub-objects into data space
    * (TRI_shape_length_list_t*) ptr = n;
    ptr += sizeof(TRI_shape_length_list_t);

    sids = (TRI_shape_sid_t*) ptr;
    ptr += n * sizeof(TRI_shape_sid_t);

    offsets = (TRI_shape_size_t*) ptr;
    ptr += (n + 1) * sizeof(TRI_shape_size_t);

    for (p = values;  p < e;  ++p) {
      *sids++ = p->_sid;

      *offsets++ = offset;
      offset += p->_size;

      memcpy(ptr, p->_value, p->_size);
      ptr += p->_size;
    }

    *offsets = offset;
  }

  // free TRI_shape_value_t array
  for (p = values;  p < e;  ++p) {
    if (p->_value != 0) {
      TRI_Free(shaper->_memoryZone, p->_value);
    }
  }

  TRI_Free(shaper->_memoryZone, values);
  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a json array into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueArray (TRI_shaper_t* shaper,
                                TRI_shape_value_t* dst,
                                v8::Handle<v8::Object> const& json,
                                set<int>& seenHashes,
                                vector< v8::Handle<v8::Object> >& seenObjects,
                                bool create,
                                bool isLocked) {
  size_t i, n;
  size_t total;

  size_t f;
  size_t v;

  TRI_shape_value_t* values;
  TRI_shape_value_t* p;
  TRI_shape_value_t* e;

  TRI_array_shape_t* a;

  TRI_shape_sid_t* sids;
  TRI_shape_aid_t* aids;
  TRI_shape_size_t* offsetsF;
  TRI_shape_size_t* offsetsV;
  TRI_shape_size_t offset;

  TRI_shape_t const* found;

  char* ptr;

  // number of attributes
  v8::Handle<v8::Array> names = json->GetOwnPropertyNames();
  n = names->Length();

  // convert into TRI_shape_value_t array
  p = (values = (TRI_shape_value_t*) TRI_Allocate(shaper->_memoryZone, n * sizeof(TRI_shape_value_t), true));

  if (p == 0) {
    return TRI_ERROR_OUT_OF_MEMORY;
  }

  total = 0;
  f = 0;
  v = 0;
  
  for (i = 0;  i < n;  ++i, ++p) {
    v8::Handle<v8::Value> key = names->Get(i);
    v8::Handle<v8::Value> val = json->Get(key);
    int res;

    // first find an identifier for the name
    TRI_Utf8ValueNFC keyStr(TRI_UNKNOWN_MEM_ZONE, key);

    if (*keyStr == 0 || keyStr.length() == 0) {
      --p;
      continue;
    }

    if ((*keyStr)[0] == '_') {
      --p;
      continue;
    }

    if (create) {
      p->_aid = shaper->findOrCreateAttributeByName(shaper, *keyStr, isLocked);
    }
    else {
      p->_aid = shaper->lookupAttributeByName(shaper, *keyStr);
    }

    // convert value
    if (p->_aid == 0) {
      if (create) {
        res = TRI_ERROR_INTERNAL;
      }
      else {
        res = TRI_RESULT_ELEMENT_NOT_FOUND;
      }
    }
    else {
      res = FillShapeValueJson(shaper, p, val, seenHashes, seenObjects, create, isLocked);
    }

    if (res != TRI_ERROR_NO_ERROR) {
      for (e = p, p = values;  p < e;  ++p) {
        if (p->_value != 0) {
          TRI_Free(shaper->_memoryZone, p->_value);
        }
      }

      TRI_Free(shaper->_memoryZone, values);
      return res;
    }

    total += p->_size;

    // count fixed and variable sized values
    if (p->_fixedSized) {
      ++f;
    }
    else {
      ++v;
    }
  }

  // adjust n
  n = f + v;

  // add variable offset table size
  total += (v + 1) * sizeof(TRI_shape_size_t);

  // now sort the shape entries
  if (n > 1) {
    TRI_SortShapeValues(values, n);
  }

#ifdef DEBUG_JSON_SHAPER
  printf("shape values\n------------\ntotal: %u, fixed: %u, variable: %u\n",
         (unsigned int) n,
         (unsigned int) f,
         (unsigned int) v);
  TRI_PrintShapeValues(values, n);
  printf("\n");
#endif

  // generate shape structure
  i =
    sizeof(TRI_array_shape_t)
    + n * sizeof(TRI_shape_sid_t)
    + n * sizeof(TRI_shape_aid_t)
    + (f + 1) * sizeof(TRI_shape_size_t);

  a = (TRI_array_shape_t*) (ptr = (char*) TRI_Allocate(shaper->_memoryZone, i, true));

  if (ptr == NULL) {
    e = values + n;

    for (p = values;  p < e;  ++p) {
      if (p->_value != NULL) {
        TRI_Free(shaper->_memoryZone, p->_value);
      }
    }

    TRI_Free(shaper->_memoryZone, values);

    return TRI_ERROR_OUT_OF_MEMORY;
  }

  a->base._type = TRI_SHAPE_ARRAY;
  a->base._size = i;
  a->base._dataSize = (v == 0) ? total : TRI_SHAPE_SIZE_VARIABLE;

  a->_fixedEntries = f;
  a->_variableEntries = v;

  ptr += sizeof(TRI_array_shape_t);

  // array of shape identifiers
  sids = (TRI_shape_sid_t*) ptr;
  ptr += n * sizeof(TRI_shape_sid_t);

  // array of attribute identifiers
  aids = (TRI_shape_aid_t*) ptr;
  ptr += n * sizeof(TRI_shape_aid_t);

  // array of offsets for fixed part (within the shape)
  offset = (v + 1) * sizeof(TRI_shape_size_t);
  offsetsF = (TRI_shape_size_t*) ptr;

  // fill destination (except sid)
  dst->_type = TRI_SHAPE_ARRAY;

  dst->_fixedSized = true;
  dst->_size = total;
  dst->_value = (ptr = (char*) TRI_Allocate(shaper->_memoryZone, dst->_size, true));

  if (ptr == 0) {
    e = values + n;

    for (p = values;  p < e;  ++p) {
      if (p->_value != 0) {
        TRI_Free(shaper->_memoryZone, p->_value);
      }
    }

    TRI_Free(shaper->_memoryZone, values);
    TRI_Free(shaper->_memoryZone, a);

    return TRI_ERROR_OUT_OF_MEMORY;
  }

  // array of offsets for variable part (within the value)
  offsetsV = (TRI_shape_size_t*) ptr;
  ptr += (v + 1) * sizeof(TRI_shape_size_t);

  // and fill in attributes
  e = values + n;

  for (p = values;  p < e;  ++p) {
    *aids++ = p->_aid;
    *sids++ = p->_sid;

    memcpy(ptr, p->_value, p->_size);
    ptr += p->_size;

    dst->_fixedSized &= p->_fixedSized;

    if (p->_fixedSized) {
      *offsetsF++ = offset;
      offset += p->_size;
      *offsetsF = offset;
    }
    else {
      *offsetsV++ = offset;
      offset += p->_size;
      *offsetsV = offset;
    }
  }

  // free TRI_shape_value_t array
  for (p = values;  p < e;  ++p) {
    if (p->_value != 0) {
      TRI_Free(shaper->_memoryZone, p->_value);
    }
  }

  TRI_Free(shaper->_memoryZone, values);

  // lookup this shape
  found = shaper->findShape(shaper, &a->base, create, isLocked);

  if (found == 0) {
    LOG_TRACE("shaper failed to find shape %d", (int) a->base._type);
    TRI_Free(shaper->_memoryZone, a);

    if (! create) {
      return TRI_RESULT_ELEMENT_NOT_FOUND;
    }

    return TRI_ERROR_INTERNAL;
  }

  // and finally add the sid
  dst->_sid = found->_sid;
  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a json object into TRI_shape_value_t
////////////////////////////////////////////////////////////////////////////////

static int FillShapeValueJson (TRI_shaper_t* shaper,
                               TRI_shape_value_t* dst,
                               v8::Handle<v8::Value> const& json,
                               set<int>& seenHashes,
                               vector< v8::Handle<v8::Object> >& seenObjects,
                               bool create,
                               bool isLocked) {
  // check for cycles
  if (json->IsObject()) {
    v8::Handle<v8::Object> o = json->ToObject();
    int hash = o->GetIdentityHash();

    if (seenHashes.find(hash) != seenHashes.end()) {
      // LOG_TRACE("found hash %d", hash);

      for (vector< v8::Handle<v8::Object> >::iterator i = seenObjects.begin();  i != seenObjects.end();  ++i) {
        if (json->StrictEquals(*i)) {
          LOG_TRACE("found duplicate for hash %d", hash);
          return TRI_ERROR_ARANGO_SHAPER_FAILED;
        }
      }
    }
    else {
      seenHashes.insert(hash);
    }

    seenObjects.push_back(o);
  }

  if (json->IsNull()) {
    return FillShapeValueNull(shaper, dst);
  }

  else if (json->IsBoolean()) {
    return FillShapeValueBoolean(shaper, dst, json->ToBoolean());
  }

  else if (json->IsBooleanObject()) {
    return FillShapeValueBoolean(shaper, dst, v8::Handle<v8::BooleanObject>::Cast(json));
  }

  else if (json->IsNumber()) {
    return FillShapeValueNumber(shaper, dst, json->ToNumber());
  }

  else if (json->IsNumberObject()) {
    return FillShapeValueNumber(shaper, dst, v8::Handle<v8::NumberObject>::Cast(json));
  }

  else if (json->IsString()) {
    return FillShapeValueString(shaper, dst, json->ToString());
  }

  else if (json->IsStringObject()) {
    return FillShapeValueString(shaper, dst, v8::Handle<v8::StringObject>::Cast(json)->StringValue());
  }

  else if (json->IsArray()) {
    return FillShapeValueList(shaper, dst, v8::Handle<v8::Array>::Cast(json), seenHashes, seenObjects, create, isLocked);
  }
   
  else if (json->IsObject()) { 
    int res = FillShapeValueArray(shaper, dst, json->ToObject(), seenHashes, seenObjects, create, isLocked);
    seenObjects.pop_back();
    return res;
  }

  else if (json->IsRegExp()) {
    LOG_TRACE("shaper failed because a regexp cannot be converted");
    return TRI_ERROR_BAD_PARAMETER;
  }

  else if (json->IsFunction()) {
    LOG_TRACE("shaper failed because a function cannot be converted");
    return TRI_ERROR_BAD_PARAMETER;
  }

  else if (json->IsExternal()) {
    LOG_TRACE("shaper failed because an external cannot be converted");
    return TRI_ERROR_BAD_PARAMETER;
  }

  else if (json->IsDate()) {
    LOG_TRACE("shaper failed because a date cannot be converted");
    return TRI_ERROR_BAD_PARAMETER;
  }

  // treat undefined as null value
  else if (json->IsUndefined()) {
    return FillShapeValueNull(shaper, dst);
  }

  LOG_TRACE("shaper failed to convert object");
  return TRI_ERROR_BAD_PARAMETER;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data null blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataNull (TRI_shaper_t* shaper,
                                                TRI_shape_t const* shape,
                                                char const* data,
                                                size_t size) {
  return v8::Null();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data boolean blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataBoolean (TRI_shaper_t* shaper,
                                                   TRI_shape_t const* shape,
                                                   char const* data,
                                                   size_t size) {
  bool v;

  v = (* (TRI_shape_boolean_t const*) data) != 0;

  return v8::Boolean::New(v);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data number blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataNumber (TRI_shaper_t* shaper,
                                                  TRI_shape_t const* shape,
                                                  char const* data,
                                                  size_t size) {
  TRI_shape_number_t v;

  v = * (TRI_shape_number_t const*) data;

  return v8::Number::New(v);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data short string blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataShortString (TRI_shaper_t* shaper,
                                                       TRI_shape_t const* shape,
                                                       char const* data,
                                                       size_t size) {
  TRI_shape_length_short_string_t l;

  l = * (TRI_shape_length_short_string_t const*) data;
  data += sizeof(TRI_shape_length_short_string_t);

  return v8::String::New(data, l - 1);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data long string blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataLongString (TRI_shaper_t* shaper,
                                                      TRI_shape_t const* shape,
                                                      char const* data,
                                                      size_t size) {
  TRI_shape_length_long_string_t l;

  l = * (TRI_shape_length_long_string_t const*) data;
  data += sizeof(TRI_shape_length_long_string_t);

  return v8::String::New(data, l - 1);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data array blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataArray (TRI_shaper_t* shaper,
                                                 TRI_shape_t const* shape,
                                                 char const* data,
                                                 size_t size) {
  TRI_array_shape_t const* s;
  TRI_shape_aid_t const* aids;
  TRI_shape_sid_t const* sids;
  TRI_shape_size_t const* offsetsF;
  TRI_shape_size_t const* offsetsV;
  TRI_shape_size_t f;
  TRI_shape_size_t i;
  TRI_shape_size_t n;
  TRI_shape_size_t v;
  shape_cache_t shapeCache;
  char const* qtr;

  v8::Handle<v8::Object> array;

  s = (TRI_array_shape_t const*) shape;
  f = s->_fixedEntries;
  v = s->_variableEntries;
  n = f + v;

  qtr = (char const*) shape;
  array = v8::Object::New();

  qtr += sizeof(TRI_array_shape_t);

  sids = (TRI_shape_sid_t const*) qtr;
  qtr += n * sizeof(TRI_shape_sid_t);

  aids = (TRI_shape_aid_t const*) qtr;
  qtr += n * sizeof(TRI_shape_aid_t);

  offsetsF = (TRI_shape_size_t const*) qtr;
  shapeCache._sid   = 0;
  shapeCache._shape = 0;

  for (i = 0;  i < f;  ++i, ++sids, ++aids, ++offsetsF) {
    TRI_shape_sid_t sid = *sids;

    TRI_shape_t const* subshape;
    if (sid == shapeCache._sid && shapeCache._sid > 0) {
      subshape = shapeCache._shape;
    }
    else {
      shapeCache._shape = subshape = shaper->lookupShapeId(shaper, sid);
      shapeCache._sid = sid;
    }

    if (subshape == 0) {
      LOG_WARNING("cannot find shape #%u", (unsigned int) sid);
      continue;
    }
    
    TRI_shape_aid_t aid = *aids;
    char const* name = shaper->lookupAttributeId(shaper, aid);

    if (name == 0) {
      LOG_WARNING("cannot find attribute #%u", (unsigned int) aid);
      continue;
    }

    const TRI_shape_size_t offset = *offsetsF;
    v8::Handle<v8::Value> element = JsonShapeData(shaper, subshape, data + offset, offsetsF[1] - offset);
    array->Set(v8::String::New(name), element);
  }

  offsetsV = (TRI_shape_size_t const*) data;

  for (i = 0;  i < v;  ++i, ++sids, ++aids, ++offsetsV) {
    TRI_shape_sid_t sid = *sids;

    TRI_shape_t const* subshape;
    if (sid == shapeCache._sid && shapeCache._sid > 0) {
      subshape = shapeCache._shape;
    }
    else {
      shapeCache._shape = subshape = shaper->lookupShapeId(shaper, sid);
      shapeCache._sid = sid;
    }

    if (subshape == 0) {
      LOG_WARNING("cannot find shape #%u", (unsigned int) sid);
      continue;
    }
    
    TRI_shape_aid_t aid = *aids;
    char const* name = shaper->lookupAttributeId(shaper, aid);

    if (name == 0) {
      LOG_WARNING("cannot find attribute #%u", (unsigned int) aid);
      continue;
    }

    const TRI_shape_size_t offset = *offsetsV;
    v8::Handle<v8::Value> element = JsonShapeData(shaper, subshape, data + offset, offsetsV[1] - offset);
    array->Set(v8::String::New(name), element);
  }

  return array;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data list blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataList (TRI_shaper_t* shaper,
                                                TRI_shape_t const* shape,
                                                char const* data,
                                                size_t size) {
  TRI_shape_length_list_t i;
  TRI_shape_length_list_t l;
  TRI_shape_sid_t const* sids;
  TRI_shape_size_t const* offsets;
  shape_cache_t shapeCache;
  char const* ptr;
  v8::Handle<v8::Array> list;

  ptr = data;
  l = * (TRI_shape_length_list_t const*) ptr;

  if (l == 0) {
    return v8::Array::New(l);
  }
 
  shapeCache._sid   = 0;
  shapeCache._shape = 0;
  list = v8::Array::New(l);

  ptr += sizeof(TRI_shape_length_list_t);
  sids = (TRI_shape_sid_t const*) ptr;

  ptr += l * sizeof(TRI_shape_sid_t);
  offsets = (TRI_shape_size_t const*) ptr;
  
  for (i = 0;  i < l;  ++i, ++sids, ++offsets) {
    TRI_shape_sid_t sid = *sids;

    TRI_shape_t const* subshape;
    if (sid == shapeCache._sid && shapeCache._sid > 0) {
      subshape = shapeCache._shape;
    }
    else {
      shapeCache._shape = subshape = shaper->lookupShapeId(shaper, sid);
      shapeCache._sid   = sid;
    }

    if (subshape == 0) {
      LOG_WARNING("cannot find shape #%u", (unsigned int) sid);
      continue;
    }

    const TRI_shape_size_t offset = *offsets;
    v8::Handle<v8::Value> element = JsonShapeData(shaper, subshape, data + offset, offsets[1] - offset);
    list->Set(i, element);
  }

  return list;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data homogeneous list blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataHomogeneousList (TRI_shaper_t* shaper,
                                                           TRI_shape_t const* shape,
                                                           char const* data,
                                                           size_t size) {
  TRI_homogeneous_list_shape_t const* s;
  TRI_shape_length_list_t i;
  TRI_shape_length_list_t l;
  TRI_shape_sid_t sid;
  TRI_shape_size_t const* offsets;
  char const* ptr;
  TRI_shape_t const* subshape;
  v8::Handle<v8::Array> list;

  s = (TRI_homogeneous_list_shape_t const*) shape;
  sid = s->_sidEntry;

  ptr = data;
  l = * (TRI_shape_length_list_t const*) ptr;

  ptr += sizeof(TRI_shape_length_list_t);
  offsets = (TRI_shape_size_t const*) ptr;
  
  subshape = shaper->lookupShapeId(shaper, sid);

  if (subshape == 0) {
    LOG_WARNING("cannot find shape #%u", (unsigned int) sid);
    return v8::Array::New();
  }
  
  list = v8::Array::New(l);

  for (i = 0;  i < l;  ++i, ++offsets) {
    TRI_shape_size_t offset = *offsets;
    v8::Handle<v8::Value> element = JsonShapeData(shaper, subshape, data + offset, offsets[1] - offset);

    list->Set(i, element);
  }

  return list;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data homogeneous sized list blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeDataHomogeneousSizedList (TRI_shaper_t* shaper,
                                                                TRI_shape_t const* shape,
                                                                char const* data,
                                                                size_t size) {
  TRI_homogeneous_sized_list_shape_t const* s;
  TRI_shape_length_list_t i;
  TRI_shape_length_list_t l;
  TRI_shape_sid_t sid;
  TRI_shape_size_t length;
  char const* ptr;
  TRI_shape_t const* subshape;
  v8::Handle<v8::Array> list;

  s = (TRI_homogeneous_sized_list_shape_t const*) shape;

  ptr = data;
  l = * (TRI_shape_length_list_t const*) ptr;

  if (l == 0) {
    return v8::Array::New();
  }
  
  sid    = s->_sidEntry;
  length = s->_sizeEntry;
  
  subshape = shaper->lookupShapeId(shaper, sid);

  if (subshape == 0) {
    LOG_WARNING("cannot find shape #%u", (unsigned int) sid);
    return v8::Array::New();
  }

  TRI_shape_size_t offset = sizeof(TRI_shape_length_list_t);
  list = v8::Array::New(l);

  for (i = 0;  i < l;  ++i, offset += length) {
    v8::Handle<v8::Value> element = JsonShapeData(shaper, subshape, data + offset, length);
    list->Set(i, element);
  }

  return list;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a data blob into a json object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JsonShapeData (TRI_shaper_t* shaper,
                                            TRI_shape_t const* shape,
                                            char const* data,
                                            size_t size) {
  if (shape == 0) {
    return v8::Null();
  }

  switch (shape->_type) {
    case TRI_SHAPE_NULL:
      return JsonShapeDataNull(shaper, shape, data, size);

    case TRI_SHAPE_BOOLEAN:
      return JsonShapeDataBoolean(shaper, shape, data, size);

    case TRI_SHAPE_NUMBER:
      return JsonShapeDataNumber(shaper, shape, data, size);

    case TRI_SHAPE_SHORT_STRING:
      return JsonShapeDataShortString(shaper, shape, data, size);

    case TRI_SHAPE_LONG_STRING:
      return JsonShapeDataLongString(shaper, shape, data, size);

    case TRI_SHAPE_ARRAY:
      return JsonShapeDataArray(shaper, shape, data, size);

    case TRI_SHAPE_LIST:
      return JsonShapeDataList(shaper, shape, data, size);

    case TRI_SHAPE_HOMOGENEOUS_LIST:
      return JsonShapeDataHomogeneousList(shaper, shape, data, size);

    case TRI_SHAPE_HOMOGENEOUS_SIZED_LIST:
      return JsonShapeDataHomogeneousSizedList(shaper, shape, data, size);
  }

  return v8::Null();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_json_t NULL into a V8 object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> ObjectJsonNull (TRI_json_t const* json) {
  return v8::Null();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_json_t BOOLEAN into a V8 object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> ObjectJsonBoolean (TRI_json_t const* json) {
  return v8::Boolean::New(json->_value._boolean);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_json_t NUMBER into a V8 object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> ObjectJsonNumber (TRI_json_t const* json) {
  return v8::Number::New(json->_value._number);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_json_t NUMBER into a V8 object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> ObjectJsonString (TRI_json_t const* json) {
  return v8::String::New(json->_value._string.data, json->_value._string.length - 1);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_json_t ARRAY into a V8 object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> ObjectJsonArray (TRI_json_t const* json) {
  v8::Handle<v8::Object> object = v8::Object::New();

  const size_t n = json->_value._objects._length;

  for (size_t i = 0;  i < n;  i += 2) {
    TRI_json_t const* key = (TRI_json_t const*) TRI_AtVector(&json->_value._objects, i);

    if (! TRI_IsStringJson(key)) {
      continue;
    }

    TRI_json_t const* j = (TRI_json_t const*) TRI_AtVector(&json->_value._objects, i + 1);
    v8::Handle<v8::Value> val = TRI_ObjectJson(j);

    object->Set(v8::String::New(key->_value._string.data, key->_value._string.length - 1), val);
  }

  return object;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_json_t LIST into a V8 object
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> ObjectJsonList (TRI_json_t const* json) {
  const size_t n = json->_value._objects._length;
  
  v8::Handle<v8::Array> object = v8::Array::New(n);

  for (size_t i = 0;  i < n;  ++i) {
    TRI_json_t const* j = (TRI_json_t const*) TRI_AtVector(&json->_value._objects, i);
    v8::Handle<v8::Value> val = TRI_ObjectJson(j);

    object->Set(i, val);
  }

  return object;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief pushes the names of an associative char* array into a V8 array
////////////////////////////////////////////////////////////////////////////////

v8::Handle<v8::Array> TRI_ArrayAssociativePointer (TRI_associative_pointer_t const* array) {
  v8::HandleScope scope;
  v8::Handle<v8::Array> result = v8::Array::New();

  uint32_t j = 0;
  uint32_t n = (uint32_t) array->_nrAlloc;

  for (uint32_t i = 0;  i < n;  ++i) {
    char* value = (char*) array->_table[i];

    if (value == 0) {
      continue;
    }

    result->Set(j++, v8::String::New(value));
  }

  return scope.Close(result);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_json_t into a V8 object
////////////////////////////////////////////////////////////////////////////////

v8::Handle<v8::Value> TRI_ObjectJson (TRI_json_t const* json) {
  v8::HandleScope scope;

  if (json == 0) {
    return scope.Close(v8::Undefined());
  }

  switch (json->_type) {
    case TRI_JSON_UNUSED:
      return scope.Close(v8::Undefined());

    case TRI_JSON_NULL:
      return scope.Close(ObjectJsonNull(json));

    case TRI_JSON_BOOLEAN:
      return scope.Close(ObjectJsonBoolean(json));

    case TRI_JSON_NUMBER:
      return scope.Close(ObjectJsonNumber(json));

    case TRI_JSON_STRING:
    case TRI_JSON_STRING_REFERENCE:
      return scope.Close(ObjectJsonString(json));

    case TRI_JSON_ARRAY:
      return scope.Close(ObjectJsonArray(json));

    case TRI_JSON_LIST:
      return scope.Close(ObjectJsonList(json));
  }

  return scope.Close(v8::Undefined());
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a TRI_shaped_json_t into a V8 object
////////////////////////////////////////////////////////////////////////////////

v8::Handle<v8::Value> TRI_JsonShapeData (TRI_shaper_t* shaper,
                                         TRI_shape_t const* shape,
                                         char const* data,
                                         size_t size) {
  return JsonShapeData(shaper, shape, data, size);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a V8 object to a TRI_shaped_json_t
////////////////////////////////////////////////////////////////////////////////

TRI_shaped_json_t* TRI_ShapedJsonV8Object (v8::Handle<v8::Value> const& object, 
                                           TRI_shaper_t* shaper,
                                           bool create,
                                           bool isLocked) {
  TRI_shape_value_t dst;
  set<int> seenHashes;
  vector< v8::Handle<v8::Object> > seenObjects;

  int res = FillShapeValueJson(shaper, &dst, object, seenHashes, seenObjects, create, isLocked);

  if (res != TRI_ERROR_NO_ERROR) {
    if (res == TRI_RESULT_ELEMENT_NOT_FOUND) {
      TRI_set_errno(res);
    }
    else {
      TRI_set_errno(TRI_ERROR_ARANGO_SHAPER_FAILED);
    }
    return 0;
  }

  TRI_shaped_json_t* shaped = (TRI_shaped_json_t*) TRI_Allocate(shaper->_memoryZone, sizeof(TRI_shaped_json_t), false);

  if (shaped == NULL) {
    return NULL;
  }

  shaped->_sid = dst._sid;
  shaped->_data.length = dst._size;
  shaped->_data.data = dst._value;

  return shaped;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts a V8 object to a TRI_shaped_json_t in place
////////////////////////////////////////////////////////////////////////////////

int TRI_FillShapedJsonV8Object (v8::Handle<v8::Value> const& object,
                                TRI_shaped_json_t* result,
                                TRI_shaper_t* shaper,
                                bool create,
                                bool isLocked) {
  TRI_shape_value_t dst;
  set<int> seenHashes;
  vector< v8::Handle<v8::Object> > seenObjects;

  int res = FillShapeValueJson(shaper, &dst, object, seenHashes, seenObjects, create, isLocked);

  if (res != TRI_ERROR_NO_ERROR) {
    if (res != TRI_RESULT_ELEMENT_NOT_FOUND) {
      res = TRI_ERROR_BAD_PARAMETER;
    }
    return TRI_set_errno(res);
  }

  result->_sid = dst._sid;
  result->_data.length = dst._size;
  result->_data.data = dst._value;

  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert a V8 value to a json_t value
////////////////////////////////////////////////////////////////////////////////

static TRI_json_t* ObjectToJson (v8::Handle<v8::Value> const& parameter,
                                 set<int>& seenHashes,
                                 vector<v8::Handle<v8::Object> >& seenObjects) {
  if (parameter->IsBoolean()) {
    v8::Handle<v8::Boolean> booleanParameter = parameter->ToBoolean();
    return TRI_CreateBooleanJson(TRI_UNKNOWN_MEM_ZONE, booleanParameter->Value());
  }

  if (parameter->IsNull()) {
    return TRI_CreateNullJson(TRI_UNKNOWN_MEM_ZONE);
  }

  if (parameter->IsNumber()) {
    v8::Handle<v8::Number> numberParameter = parameter->ToNumber();
    return TRI_CreateNumberJson(TRI_UNKNOWN_MEM_ZONE, numberParameter->Value());
  }

  if (parameter->IsString()) {
    v8::Handle<v8::String> stringParameter= parameter->ToString();
    TRI_Utf8ValueNFC str(TRI_UNKNOWN_MEM_ZONE, stringParameter);
    // move the string pointer into the JSON object
    if (*str != 0) {
      TRI_json_t* j = TRI_CreateString2Json(TRI_UNKNOWN_MEM_ZONE, *str, str.length());
      // this passes ownership for the utf8 string to the JSON object
      str.steal();
    
      // the Utf8ValueNFC dtor won't free the string now
      return j;
    }

    return 0;
  }

  if (parameter->IsArray()) {
    v8::Handle<v8::Array> arrayParameter = v8::Handle<v8::Array>::Cast(parameter);
    const uint32_t n = arrayParameter->Length();

    TRI_json_t* listJson = TRI_CreateList2Json(TRI_UNKNOWN_MEM_ZONE, (const size_t) n);

    if (listJson != 0) {
      for (uint32_t j = 0; j < n; ++j) {
        v8::Handle<v8::Value> item = arrayParameter->Get(j);
        TRI_json_t* result = ObjectToJson(item, seenHashes, seenObjects);

        if (result != 0) {
          TRI_PushBack3ListJson(TRI_UNKNOWN_MEM_ZONE, listJson, result);
        }
      }
    }
    return listJson;
  }

  if (parameter->IsObject()) {
    v8::Handle<v8::Object> o = parameter->ToObject();
    int hash = o->GetIdentityHash();

    if (seenHashes.find(hash) != seenHashes.end()) {
      // LOG_TRACE("found hash %d", hash);

      for (vector< v8::Handle<v8::Object> >::iterator i = seenObjects.begin();  i != seenObjects.end();  ++i) {
        if (parameter->StrictEquals(*i)) {
          LOG_TRACE("found duplicate for hash %d", hash);
          return 0;
        }
      }
    }
    else {
      seenHashes.insert(hash);
    }

    seenObjects.push_back(o);

    v8::Handle<v8::Array> arrayParameter = v8::Handle<v8::Array>::Cast(parameter);
    v8::Handle<v8::Array> names = arrayParameter->GetOwnPropertyNames();
    const uint32_t n = names->Length();

    TRI_json_t* arrayJson = TRI_CreateArray2Json(TRI_UNKNOWN_MEM_ZONE, (const size_t) n);

    if (arrayJson != 0 && n > 0) {
      for (uint32_t j = 0; j < n; ++j) {
        v8::Handle<v8::Value> key  = names->Get(j);
        v8::Handle<v8::Value> item = arrayParameter->Get(key);
        TRI_json_t* result = ObjectToJson(item, seenHashes, seenObjects);
    
        if (result != 0) {
          TRI_Utf8ValueNFC str(TRI_UNKNOWN_MEM_ZONE, key);

          if (*str != 0) {
            // move the string pointer into the JSON object
            TRI_Insert4ArrayJson(TRI_UNKNOWN_MEM_ZONE, arrayJson, *str, str.length(), result, false);
            // this passes ownership for the utf8 string to the JSON object
            str.steal();
          }

          TRI_Free(TRI_UNKNOWN_MEM_ZONE, result);
        }
      }
    }
    
    seenObjects.pop_back();

    return arrayJson;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert a V8 value to a json_t value
////////////////////////////////////////////////////////////////////////////////

TRI_json_t* TRI_ObjectToJson (v8::Handle<v8::Value> const& parameter) {
  set<int> seenHashes;
  vector< v8::Handle<v8::Object> > seenObjects;

  return ObjectToJson(parameter, seenHashes, seenObjects);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts an V8 object to a string
////////////////////////////////////////////////////////////////////////////////

string TRI_ObjectToString (v8::Handle<v8::Value> const& value) {
  TRI_Utf8ValueNFC utf8Value(TRI_UNKNOWN_MEM_ZONE, value);

  if (*utf8Value == 0) {
    return "";
  }
  else {
    return string(*utf8Value, utf8Value.length());
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts an V8 object to a character
////////////////////////////////////////////////////////////////////////////////

char TRI_ObjectToCharacter (v8::Handle<v8::Value> const& value, bool& error) {
  error = false;

  if (! value->IsString() && ! value->IsStringObject()) {
    error = true;
    return '\0';
  }

  TRI_Utf8ValueNFC sep(TRI_UNKNOWN_MEM_ZONE, value->ToString());

  if (*sep == 0 || sep.length() != 1) {
    error = true;
    return '\0';
  }

  return (*sep)[0];
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts an V8 object to an int64_t
////////////////////////////////////////////////////////////////////////////////

int64_t TRI_ObjectToInt64 (v8::Handle<v8::Value> const& value) {
  if (value->IsNumber()) {
    return (int64_t) value->ToNumber()->Value();
  }

  if (value->IsNumberObject()) {
    v8::Handle<v8::NumberObject> no = v8::Handle<v8::NumberObject>::Cast(value);
    return (int64_t) no->NumberValue();
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts an V8 object to a uint64_t
////////////////////////////////////////////////////////////////////////////////

uint64_t TRI_ObjectToUInt64 (v8::Handle<v8::Value> const& value,
                             const bool allowStringConversion) {
  if (value->IsNumber()) {
    return (uint64_t) value->ToNumber()->Value();
  }

  if (value->IsNumberObject()) {
    v8::Handle<v8::NumberObject> no = v8::Handle<v8::NumberObject>::Cast(value);
    return (uint64_t) no->NumberValue();
  }

  if (allowStringConversion && value->IsString()) {
    v8::String::Utf8Value str(value);
    return StringUtils::uint64(string(*str, str.length()));
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts an V8 object to a double
////////////////////////////////////////////////////////////////////////////////

double TRI_ObjectToDouble (v8::Handle<v8::Value> const& value) {
  if (value->IsNumber()) {
    return value->ToNumber()->Value();
  }

  if (value->IsNumberObject()) {
    v8::Handle<v8::NumberObject> no = v8::Handle<v8::NumberObject>::Cast(value);
    return no->NumberValue();
  }

  return 0.0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts an V8 object to a double with error handling
////////////////////////////////////////////////////////////////////////////////

double TRI_ObjectToDouble (v8::Handle<v8::Value> const& value, bool& error) {
  error = false;

  if (value->IsNumber()) {
    return value->ToNumber()->Value();
  }

  if (value->IsNumberObject()) {
    v8::Handle<v8::NumberObject> no = v8::Handle<v8::NumberObject>::Cast(value);
    return no->NumberValue();
  }

  error = true;

  return 0.0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief converts an V8 object to a boolean
////////////////////////////////////////////////////////////////////////////////

bool TRI_ObjectToBoolean (v8::Handle<v8::Value> const& value) {
  if (value->IsBoolean()) {
    return value->ToBoolean()->Value();
  }
  else if (value->IsBooleanObject()) {
    v8::Handle<v8::BooleanObject> bo = v8::Handle<v8::BooleanObject>::Cast(value);
    return bo->BooleanValue();
  }

  return false;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief initialises the V8 conversion module
////////////////////////////////////////////////////////////////////////////////

void TRI_InitV8Conversions (v8::Handle<v8::Context> context) {
  // nothing special to do here
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
