#pragma once
#include "stdafx.h"
#include "KinectStreamDescription.h"
#include "KinectMediaStream.h"

namespace k4u
{
  class __declspec(uuid("{82bd2aa2-a114-45f8-80dd-92fab9dbcc42}")) KinectMediaSource : public winrt::implements<
    KinectMediaSource,
    IMFMediaEventGenerator,
    IMFMediaSource,
    IMFMediaSourceEx,
    IMFGetService,
    IKsControl>
  {
  public:
    KinectMediaSource();
    ~KinectMediaSource();

    //IMFMediaEventGenerator
    virtual HRESULT __stdcall BeginGetEvent(IMFAsyncCallback* callback, IUnknown* state) noexcept override;
    virtual HRESULT __stdcall EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** event) noexcept override;

    virtual HRESULT __stdcall GetEvent(DWORD flags, IMFMediaEvent** event) noexcept override;
    virtual HRESULT __stdcall QueueEvent(MediaEventType type, const GUID& extendedType, HRESULT status, const PROPVARIANT* value) noexcept override;

    //IMFMediaSource
    virtual HRESULT __stdcall CreatePresentationDescriptor(IMFPresentationDescriptor** presentationDescriptor) noexcept override;
    virtual HRESULT __stdcall GetCharacteristics(DWORD* characteristics) noexcept override;
    virtual HRESULT __stdcall Pause() noexcept override;
    virtual HRESULT __stdcall Shutdown() noexcept override;
    virtual HRESULT __stdcall Start(IMFPresentationDescriptor* presentationDescriptor, const GUID* timeFormat, const PROPVARIANT* startPosition) noexcept override;
    virtual HRESULT __stdcall Stop() noexcept override;

    //IMFMediaSourceEx
    virtual HRESULT __stdcall GetSourceAttributes(IMFAttributes** attributes) noexcept override;
    virtual HRESULT __stdcall GetStreamAttributes(DWORD streamIdentifier, IMFAttributes** attributes) noexcept override;
    virtual HRESULT __stdcall SetD3DManager(IUnknown* manager) noexcept override;

    //IMFGetService
    virtual HRESULT __stdcall GetService(const GUID& guidService, const IID& riid, LPVOID* ppvObject) noexcept override;

    //IKsControl
    virtual HRESULT __stdcall KsProperty(PKSPROPERTY property, ULONG propertyLength, LPVOID propertyData, ULONG dataLength, ULONG* bytesReturned) noexcept override;
    virtual HRESULT __stdcall KsMethod(PKSPROPERTY property, ULONG propertyLength, LPVOID propertyData, ULONG dataLength, ULONG* bytesReturned) noexcept override;
    virtual HRESULT __stdcall KsEvent(PKSPROPERTY property, ULONG propertyLength, LPVOID propertyData, ULONG dataLength, ULONG* bytesReturned) noexcept override;

  private:
    std::mutex _mutex;
    winrt::com_ptr<IMFMediaEventQueue> _eventQueue;
    winrt::com_ptr<IMFPresentationDescriptor> _presentationDescriptor;
    winrt::com_ptr<IMFAttributes> _attributes;
    std::unordered_map<KinectStreamType, winrt::com_ptr<KinectMediaStream>> _streams;
    k4a_device_t _device{};
    std::thread _workerThread;
    bool _isShutdown = false;
    bool _isRunning = false;

    void InitializeAttributes();
    void RunCapture();
  };
}