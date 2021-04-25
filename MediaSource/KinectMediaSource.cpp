#include "stdafx.h"
#include "KinectMediaSource.h"
#include "KinectStreamDescription.h"

using namespace std;
using namespace winrt;

namespace k4u
{
  KinectMediaSource::KinectMediaSource()
  {
    check_hresult(MFCreateEventQueue(_eventQueue.put()));
    _presentationDescriptor = KinectStreamDescription::CreatePresentationDescription();

    InitializeAttributes();
  }

  KinectMediaSource::~KinectMediaSource()
  {
    Shutdown();
  }

  HRESULT __stdcall KinectMediaSource::BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;

    return _eventQueue->BeginGetEvent(callback, state);
  }

  HRESULT __stdcall KinectMediaSource::EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;

    return _eventQueue->EndGetEvent(result, event);
  }

  HRESULT __stdcall KinectMediaSource::GetEvent(DWORD flags, IMFMediaEvent** event) noexcept
  {
    com_ptr<IMFMediaEventQueue> eventQueue;
    {
      lock_guard<mutex> lock(_mutex);
      if (_isShutdown) return MF_E_SHUTDOWN;

      eventQueue = _eventQueue;
    }

    return eventQueue->GetEvent(flags, event);
  }

  HRESULT __stdcall KinectMediaSource::QueueEvent(MediaEventType type, const GUID& extendedType, HRESULT status, const PROPVARIANT* value) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;

    return _eventQueue->QueueEventParamVar(type, extendedType, status, value);
  }

  HRESULT __stdcall KinectMediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor** presentationDescriptor) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;
    if (!presentationDescriptor) return E_POINTER;

    return _presentationDescriptor->Clone(presentationDescriptor);
  }

  HRESULT __stdcall KinectMediaSource::GetCharacteristics(DWORD* characteristics) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;
    if (!characteristics) return E_POINTER;

    *characteristics = MFMEDIASOURCE_IS_LIVE;
    return S_OK;
  }

  HRESULT __stdcall KinectMediaSource::Pause() noexcept
  {
    return MF_E_INVALID_STATE_TRANSITION;
  }

  HRESULT __stdcall KinectMediaSource::Shutdown() noexcept
  {
    lock_guard<mutex> lock(_mutex);
    _isShutdown = true;

    _eventQueue = nullptr;
    _presentationDescriptor = nullptr;

    return S_OK;
  }

  HRESULT __stdcall KinectMediaSource::Start(IMFPresentationDescriptor* pPresentationDescriptor, const GUID* pguidTimeFormat, const PROPVARIANT* pvarStartPosition) noexcept
  {
    return E_NOTIMPL;
  }

  HRESULT __stdcall KinectMediaSource::Stop() noexcept
  {
    return E_NOTIMPL;
  }
  
  HRESULT __stdcall KinectMediaSource::GetSourceAttributes(IMFAttributes** attributes) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;
    if (!attributes) return E_POINTER;

    _attributes.copy_to(attributes);
    return S_OK;
  }

  HRESULT __stdcall KinectMediaSource::GetStreamAttributes(DWORD streamIdentifier, IMFAttributes** attributes) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;
    if(!attributes) return E_POINTER;

    try
    {
      unsigned long streamCount;
      check_hresult(_presentationDescriptor->GetStreamDescriptorCount(&streamCount));
      if (streamIdentifier >= streamCount) return MF_E_INVALIDSTREAMNUMBER;

      com_ptr<IMFStreamDescriptor> streamDescriptor;
      int isSelected;
      check_hresult(_presentationDescriptor->GetStreamDescriptorByIndex(streamIdentifier, &isSelected, streamDescriptor.put()));
      
      streamDescriptor.as<IMFAttributes>().copy_to(attributes);
      return S_OK;
    }
    catch (...)
    {
      return to_hresult();
    }
  }

  HRESULT __stdcall KinectMediaSource::SetD3DManager(IUnknown* manager) noexcept
  {
    return E_NOTIMPL;
  }

  HRESULT __stdcall KinectMediaSource::GetService(const GUID& guidService, const IID& riid, LPVOID* ppvObject) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;

    return MF_E_UNSUPPORTED_SERVICE;
  }

  HRESULT __stdcall KinectMediaSource::KsProperty(PKSPROPERTY property, ULONG propertyLength, LPVOID propertyData, ULONG dataLength, ULONG* bytesReturned) noexcept
  {
    return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
  }

  HRESULT __stdcall KinectMediaSource::KsMethod(PKSPROPERTY property, ULONG propertyLength, LPVOID propertyData, ULONG dataLength, ULONG* bytesReturned) noexcept
  {
    return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
  }

  HRESULT __stdcall KinectMediaSource::KsEvent(PKSPROPERTY property, ULONG propertyLength, LPVOID propertyData, ULONG dataLength, ULONG* bytesReturned) noexcept
  {
    return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
  }
  
  void KinectMediaSource::InitializeAttributes()
  {
    com_ptr<IMFSensorProfile> profile;
    check_hresult(MFCreateSensorProfile(KSCAMERAPROFILE_Legacy, 0, nullptr, profile.put()));
    profile->AddProfileFilter(0, L"((RES==;FRT<=30,1;SUT==))");
    profile->AddProfileFilter(1, L"((RES==;FRT<=30,1;SUT==))");

    com_ptr<IMFSensorProfileCollection> profileCollection;
    check_hresult(MFCreateSensorProfileCollection(profileCollection.put()));
    profileCollection->AddProfile(profile.get());

    check_hresult(MFCreateAttributes(_attributes.put(), 1));
    check_hresult(_attributes->SetUnknown(MF_DEVICEMFT_SENSORPROFILE_COLLECTION, profileCollection.get()));
  }
}