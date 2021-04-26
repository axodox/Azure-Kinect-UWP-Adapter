#include "stdafx.h"
#include "KinectMediaSource.h"

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
    _attributes = nullptr;
    _streams.clear();

    return S_OK;
  }

  HRESULT __stdcall KinectMediaSource::Start(IMFPresentationDescriptor* presentationDescriptor, const GUID* timeFormat, const PROPVARIANT* startPosition) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;
    if (_isRunning) return MF_E_INVALID_STATE_TRANSITION;
    if (!presentationDescriptor || !startPosition) return E_INVALIDARG;
    if (timeFormat != nullptr && *timeFormat != GUID_NULL) return MF_E_UNSUPPORTED_TIME_FORMAT;

    _isRunning = true;

    try
    {
      auto cameraConfiguration = KinectStreamDescription::CreateCameraConfiguration(presentationDescriptor);

      PROPVARIANT startTime;
      check_hresult(InitPropVariantFromInt64(MFGetSystemTime(), &startTime));
      check_hresult(_eventQueue->QueueEventParamVar(MESourceStarted, GUID_NULL, S_OK, &startTime));

      unsigned long streamCount;
      check_hresult(_presentationDescriptor->GetStreamDescriptorCount(&streamCount));

      unsigned long descriptorCount;
      check_hresult(presentationDescriptor->GetStreamDescriptorCount(&descriptorCount));

      for (unsigned long descriptorIndex = 0; descriptorIndex < descriptorCount; descriptorIndex++)
      {
        com_ptr<IMFStreamDescriptor> streamDescriptor;
        int isSelected;
        check_hresult(presentationDescriptor->GetStreamDescriptorByIndex(descriptorIndex, &isSelected, streamDescriptor.put()));

        unsigned long streamIndex = 0; //In our case stream index and stream ID is the same
        check_hresult(streamDescriptor->GetStreamIdentifier(&streamIndex));
        if (streamIndex >= streamCount) return MF_E_INVALIDSTREAMNUMBER;
        
        if (isSelected)
        {
          check_hresult(_presentationDescriptor->SelectStream(streamIndex));

          auto stream = make_self<KinectMediaStream>(get_strong().as<IMFMediaSource>(), streamDescriptor);
          check_hresult(_eventQueue->QueueEventParamUnk(MENewStream, GUID_NULL, S_OK, stream.as<IUnknown>().get()));
          check_hresult(stream->QueueEvent(MEStreamStarted, GUID_NULL, S_OK, &startTime));
          
          _streams.emplace((KinectStreamType)descriptorIndex, move(stream));
        }
        else
        {
          check_hresult(_presentationDescriptor->DeselectStream(streamIndex));
        }
      }

      return S_OK;
    }
    catch (...)
    {
      return to_hresult();
    }
  }

  HRESULT __stdcall KinectMediaSource::Stop() noexcept
  {
    lock_guard<mutex> lock(_mutex);
    if (_isShutdown) return MF_E_SHUTDOWN;
    if (!_isRunning) return MF_E_INVALID_STATE_TRANSITION;

    _isRunning = false;

    try
    {
      PROPVARIANT stopTime;
      check_hresult(InitPropVariantFromInt64(MFGetSystemTime(), &stopTime));

      unsigned long streamCount;
      check_hresult(_presentationDescriptor->GetStreamDescriptorCount(&streamCount));

      for (unsigned long streamIndex = 0; streamIndex < streamCount; streamIndex++)
      {
        com_ptr<IMFStreamDescriptor> streamDescriptor;
        int isSelected;
        check_hresult(_presentationDescriptor->GetStreamDescriptorByIndex(streamIndex, &isSelected, streamDescriptor.put()));

        if (isSelected)
        {
          _presentationDescriptor->DeselectStream(streamIndex);

          auto& stream = _streams.at((KinectStreamType)streamIndex);
          stream->QueueEvent(MEStreamStopped, GUID_NULL, S_OK, &stopTime);
          _streams.erase((KinectStreamType)streamIndex);
        }
      }

      check_hresult(_eventQueue->QueueEventParamVar(MESourceStopped, GUID_NULL, S_OK, &stopTime));
      return S_OK;
    }
    catch (...)
    {
      return to_hresult();
    }
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
    if (!attributes) return E_POINTER;

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