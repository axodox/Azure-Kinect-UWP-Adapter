#pragma once
#include "stdafx.h"

namespace k4u
{
  class KinectMediaSteam : public winrt::implements<
    KinectMediaSteam,
    IMFMediaEventGenerator,
    IMFMediaStream>
  {
  public:
    KinectMediaSteam(const winrt::com_ptr<IMFMediaSource>& parent, const winrt::com_ptr<IMFStreamDescriptor>& streamDescriptor);

    void Update(const k4a_image_t& image);

    //IMFMediaEventGenerator
    virtual HRESULT __stdcall BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) noexcept override;
    virtual HRESULT __stdcall EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) noexcept override;
    
    virtual HRESULT __stdcall GetEvent(DWORD flags, IMFMediaEvent** event) noexcept override;
    virtual HRESULT __stdcall QueueEvent(MediaEventType type, const GUID& extendedType, HRESULT status, const PROPVARIANT* value) noexcept override;

    //IMFMediaStream
    virtual HRESULT __stdcall GetMediaSource(IMFMediaSource** mediaSource) noexcept override;
    virtual HRESULT __stdcall GetStreamDescriptor(IMFStreamDescriptor** streamDescriptor) noexcept override;
    virtual HRESULT __stdcall RequestSample(IUnknown* token) noexcept override;

  private:
    std::mutex _mutex;
    winrt::com_ptr<IMFMediaSource> _parent;
    winrt::com_ptr<IMFMediaEventQueue> _eventQueue;
    winrt::com_ptr<IMFStreamDescriptor> _streamDescriptor;

    uint32_t _width, _height;
    winrt::guid _format;
    std::chrono::duration<uint32_t, std::nano> _sampleDuration;
    std::queue<winrt::com_ptr<IUnknown>> _sampleRequestTokens;
  };
}