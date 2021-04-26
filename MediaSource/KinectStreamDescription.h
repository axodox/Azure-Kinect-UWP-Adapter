#pragma once
#include "stdafx.h"
#include "Hasher.h"

namespace k4u
{
  enum class KinectStreamType : uint32_t
  {
    Color,
    Depth
  };

  class KinectStreamDescription
  {
  public:
    KinectStreamDescription(KinectStreamType streamType);
    operator const winrt::com_ptr<IMFStreamDescriptor>&() const;

    static winrt::com_ptr<IMFPresentationDescriptor> CreatePresentationDescription();
    static k4a_device_configuration_t CreateCameraConfiguration(IMFPresentationDescriptor* presentationDescriptor);

  private:
    struct MediaTypeKey
    {
      winrt::guid Format;
      uint32_t Width, Height;
      uint32_t Framerate;
    };

    KinectStreamType _streamType;
    std::unordered_map<MediaTypeKey, winrt::com_ptr<IMFMediaType>, trivial_hasher<MediaTypeKey>, trivial_comparer<MediaTypeKey>> _mediaTypes;
    winrt::com_ptr<IMFStreamDescriptor> _streamDescriptor;
    winrt::com_ptr<IMFMediaType> _defaultMediaType;

    void AddMediaType(const winrt::guid& format, uint32_t width, uint32_t height, uint32_t framesPerSecond);

    void AddColorMediaTypes();
    void AddDepthMediaTypes();

    std::vector<IMFMediaType*> AddMediaTypes();

    void SetAttributes();
  };
}