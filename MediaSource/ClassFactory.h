#pragma once
#include "stdafx.h"

template <typename TFactory, typename TResult>
struct
  ClassFactory : public winrt::implements<TFactory, IClassFactory>
{
  HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, GUID const& interfaceId, void** result) noexcept override
  {
    try
    {
      return winrt::make<TResult>()->QueryInterface(interfaceId, result);
    }
    catch (...)
    {
      return winrt::to_hresult();
    }
  }

  HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) noexcept override
  {
    return S_OK;
  }
};