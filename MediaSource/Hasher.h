#pragma once
#include "stdafx.h"

namespace k4u
{
  template <typename TValue>
  struct trivial_hasher
  {
    size_t operator()(const TValue& value) const
    {
      static_assert(std::is_trivially_copyable<TValue>::value, "The type must be trivially copyable for trivial hasher.");
      if constexpr (sizeof(TValue) <= sizeof(size_t)) //If the value type is smaller than size_t, return the value itself (with padding if needed)
      {
        size_t result = {};
        memcpy(&result, &value, sizeof(TValue));
        return result;
      }
      else //Otherwise call efficient MSVC FNV hash implementation
      {
        return std::_Hash_representation<TValue>(value);
      }
    }
  };

  template <typename TValue>
  struct trivial_comparer
  {
    bool operator()(const TValue& a, const TValue& b) const
    {
      static_assert(std::is_trivially_copyable<TValue>::value, "The type must be trivially copyable for trivial comparer.");
      return memcmp(&a, &b, sizeof(TValue)) == 0;
    }
  };
}