#include "stdafx.h"
#include "KinectMediaSteam.h"

using namespace std;
using namespace std::chrono;
using namespace winrt;

namespace k4u
{
  KinectMediaSteam::KinectMediaSteam(
    const winrt::com_ptr<IMFMediaSource>& parent,
    const winrt::com_ptr<IMFStreamDescriptor>& streamDescriptor) :
    _parent(parent),
    _streamDescriptor(streamDescriptor)
  {
    check_hresult(MFCreateEventQueue(_eventQueue.put()));

    com_ptr<IMFMediaTypeHandler> mediaTypeHandler;
    check_hresult(_streamDescriptor->GetMediaTypeHandler(mediaTypeHandler.put()));

    com_ptr<IMFMediaType> mediaType;
    check_hresult(mediaTypeHandler->GetCurrentMediaType(mediaType.put()));

    check_hresult(MFGetAttributeSize(mediaType.get(), MF_MT_FRAME_SIZE, &_width, &_height));
    check_hresult(mediaType->GetGUID(MF_MT_SUBTYPE, reinterpret_cast<GUID*>(&_format)));

    uint32_t framerateNumerator, framerateDenominator;
    MFGetAttributeRatio(mediaType.get(), MF_MT_FRAME_RATE, &framerateNumerator, &framerateDenominator);

    _sampleDuration = duration<uint32_t>(framerateDenominator);
    _sampleDuration /= framerateNumerator;
  }

  void KinectMediaSteam::Update(const k4a_image_t& image)
  {
    {
      lock_guard<mutex> lock(_mutex);
      if (_sampleRequestTokens.empty()) return;
    }

    //Validate image size
    auto width = k4a_image_get_width_pixels(image);
    auto height = k4a_image_get_height_pixels(image);

    if (width != _width || height != _height) throw hresult_out_of_bounds(L"The image retrieved from the Kinect is of an unexpected size!");

    //Create receiving buffer
    com_ptr<IMFMediaBuffer> buffer;
    check_hresult(MFCreate2DMediaBuffer(_width, _height, _format.Data1, false, buffer.put()));

    //Retrieve k4a data
    auto k4aStride = k4a_image_get_stride_bytes(image);
    auto k4aScanline0 = k4a_image_get_buffer(image);

    //Lock MF data
    auto buffer2D = buffer.as<IMF2DBuffer2>();
    uint8_t* mfScanline0, * mfBufferStart = nullptr;
    long mfStride;
    unsigned long mfBufferLength;
    check_hresult(buffer2D->Lock2DSize(MF2DBuffer_LockFlags_Write, &mfScanline0, &mfStride, &mfBufferStart, &mfBufferLength));

    //Perform copy
    auto rowLength = min(k4aStride, mfStride);
    for (auto row = 0u; row < _height; row++)
    {
      auto k4aData = k4aScanline0 + row * k4aStride;
      auto mfData = mfScanline0 + row * mfStride;

      memcpy(mfData, k4aData, rowLength);
    }

    check_hresult(buffer2D->Unlock2D());

    //Create sample
    com_ptr<IMFSample> sample;
    check_hresult(MFCreateSample(sample.put()));
    check_hresult(sample->AddBuffer(buffer.get()));
    check_hresult(sample->SetSampleTime(MFGetSystemTime()));
    check_hresult(sample->SetSampleDuration(_sampleDuration.count()));

    {
      lock_guard<mutex> lock(_mutex);
      auto token = move(_sampleRequestTokens.front());
      _sampleRequestTokens.pop();

      check_hresult(sample->SetUnknown(MFSampleExtension_Token, token.get()));
      check_hresult(_eventQueue->QueueEventParamUnk(MEMediaSample, GUID_NULL, S_OK, sample.get()));
    }
  }

  HRESULT __stdcall KinectMediaSteam::BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    return _eventQueue->BeginGetEvent(callback, state);
  }

  HRESULT __stdcall KinectMediaSteam::EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    return _eventQueue->EndGetEvent(result, event);
  }

  HRESULT __stdcall KinectMediaSteam::GetEvent(DWORD flags, IMFMediaEvent** event) noexcept
  {
    com_ptr<IMFMediaEventQueue> eventQueue;
    {
      lock_guard<mutex> lock(_mutex);
      eventQueue = _eventQueue;
    }

    return eventQueue->GetEvent(flags, event);
  }

  HRESULT __stdcall KinectMediaSteam::QueueEvent(MediaEventType type, const GUID& extendedType, HRESULT status, const PROPVARIANT* value) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    return _eventQueue->QueueEventParamVar(type, extendedType, status, value);
  }

  HRESULT __stdcall KinectMediaSteam::GetMediaSource(IMFMediaSource** mediaSource) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    _parent.copy_to(mediaSource);
    return S_OK;
  }

  HRESULT __stdcall KinectMediaSteam::GetStreamDescriptor(IMFStreamDescriptor** streamDescriptor) noexcept
  {
    lock_guard<mutex> lock(_mutex);
    _streamDescriptor.copy_to(streamDescriptor);
    return S_OK;
  }

  HRESULT __stdcall KinectMediaSteam::RequestSample(IUnknown* token) noexcept
  {
    lock_guard<mutex> lock(_mutex);

    if (token) token->AddRef();
    _sampleRequestTokens.emplace(com_ptr<IUnknown>(token, take_ownership_from_abi));
    return S_OK;
  }
}