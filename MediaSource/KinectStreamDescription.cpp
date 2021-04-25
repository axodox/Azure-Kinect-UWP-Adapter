#include "stdafx.h"
#include "KinectStreamDescription.h"

using namespace std;
using namespace winrt;

namespace k4u
{
  KinectStreamDescription::KinectStreamDescription(KinectStreamType streamType)
  {
    _streamType = streamType;

    auto mediaTypes = AddMediaTypes();
    check_hresult(MFCreateStreamDescriptor((DWORD)streamType, (uint32_t)mediaTypes.size(), mediaTypes.data(), _streamDescriptor.put()));

    com_ptr<IMFMediaTypeHandler> mediaTypeHandler;
    check_hresult(_streamDescriptor->GetMediaTypeHandler(mediaTypeHandler.put()));
    check_hresult(mediaTypeHandler->SetCurrentMediaType(_defaultMediaType.get()));

    SetAttributes();
  }

  winrt::com_ptr<IMFPresentationDescriptor> KinectStreamDescription::CreatePresentationDescription()
  {
    com_ptr<IMFStreamDescriptor> colorStream{ KinectStreamDescription(KinectStreamType::Color) };
    com_ptr<IMFStreamDescriptor> depthStream{ KinectStreamDescription(KinectStreamType::Depth) };

    array<IMFStreamDescriptor*, 2> streams = {
      colorStream.get(),
      depthStream.get()
    };

    com_ptr<IMFPresentationDescriptor> result;
    check_hresult(MFCreatePresentationDescriptor(streams.size(), streams.data(), result.put()));

    return result;
  }

  KinectStreamDescription::operator const winrt::com_ptr<IMFStreamDescriptor>& () const
  {
    return _streamDescriptor;
  }

  void KinectStreamDescription::AddMediaType(const winrt::guid& format, uint32_t width, uint32_t height, uint32_t framesPerSecond)
  {
    com_ptr<IMFMediaType> type;
    check_hresult(MFCreateMediaType(type.put()));

    check_hresult(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    check_hresult(type->SetGUID(MF_MT_SUBTYPE, format));
    check_hresult(type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
    check_hresult(type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
    check_hresult(MFSetAttributeSize(type.get(), MF_MT_FRAME_SIZE, width, height));
    check_hresult(MFSetAttributeRatio(type.get(), MF_MT_FRAME_RATE, framesPerSecond, 1));
    check_hresult(MFSetAttributeRatio(type.get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
    
    _mediaTypes.emplace(MediaTypeKey{ format, width, height, framesPerSecond }, move(type));
  }

  void KinectStreamDescription::AddColorMediaTypes()
  {
    vector<guid> formats = {
      MFVideoFormat_RGB32,
      MFVideoFormat_MJPG,
      MFVideoFormat_NV12,
      MFVideoFormat_YUY2
    };

    vector<pair<uint32_t, uint32_t>> resolutions = {
      { 1280, 720 },
      { 1920, 1080 },
      { 2560, 1440 },
      { 2048, 1536 },
      { 3840, 2160 },
      { 4096, 3072 }
    };

    vector<uint32_t> framerates = {
      30,
      15,
      5
    };

    for (auto& format : formats)
    {
      for (auto [width, height] : resolutions)
      {
        for (auto framerate : framerates)
        {
          AddMediaType(format, width, height, framerate);
        }

        if (format != guid(MFVideoFormat_RGB32) && 
          format != guid(MFVideoFormat_MJPG))
        {
          break;
        }
      }
    }    
  }

  void KinectStreamDescription::AddDepthMediaTypes()
  {
    vector<pair<uint32_t, uint32_t>> resolutions = {
      { 320, 288 },
      { 640, 576 },
      { 512, 512 },
      { 1024, 1024 }
    };

    vector<uint32_t> framerates = {
      30,
      15,
      5
    };

    for (auto [width, height] : resolutions)
    {
      for (auto framerate : framerates)
      {
        if (width > 512 && framerate > 15) continue;
        AddMediaType(MFVideoFormat_D16, width, height, framerate);
      }
    }
  }

  std::vector<IMFMediaType*> KinectStreamDescription::AddMediaTypes()
  {
    switch (_streamType)
    {
    case KinectStreamType::Color:
      AddColorMediaTypes();
      _defaultMediaType = _mediaTypes.at({ MFVideoFormat_NV12, 1280, 720, 30 });
      break;
    case KinectStreamType::Depth:
      AddDepthMediaTypes();
      _defaultMediaType = _mediaTypes.at({ MFVideoFormat_D16, 512, 512, 30 });
      break;
    }

    vector<IMFMediaType*> results;
    for (auto& [key, type] : _mediaTypes)
    {
      results.push_back(type.get());
    }
    return results;
  }

  void KinectStreamDescription::SetAttributes()
  {
    check_hresult(_streamDescriptor->SetGUID(MF_DEVICESTREAM_STREAM_CATEGORY, PINNAME_VIDEO_CAPTURE));
    check_hresult(_streamDescriptor->SetUINT32(MF_DEVICESTREAM_STREAM_ID, (unsigned)_streamType));
    check_hresult(_streamDescriptor->SetUINT32(MF_DEVICESTREAM_FRAMESERVER_SHARED, 1));

    switch (_streamType)
    {
    case KinectStreamType::Color:
      check_hresult(_streamDescriptor->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, _MFFrameSourceTypes::MFFrameSourceTypes_Color));
      break;
    case KinectStreamType::Depth:
      check_hresult(_streamDescriptor->SetUINT32(MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, _MFFrameSourceTypes::MFFrameSourceTypes_Depth));
      break;
    }
  }
}