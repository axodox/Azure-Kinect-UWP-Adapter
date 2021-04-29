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

  k4a_device_configuration_t KinectStreamDescription::CreateCameraConfiguration(IMFPresentationDescriptor* presentationDescriptor)
  {
    k4a_device_configuration_t configuration{};

    unsigned long streamCount;
    check_hresult(presentationDescriptor->GetStreamDescriptorCount(&streamCount));

    uint32_t maxFps = 0;
    for (unsigned long streamId = 0; streamId < streamCount; streamId++)
    {
      com_ptr<IMFStreamDescriptor> streamDescriptor;
      int isSelected;
      check_hresult(presentationDescriptor->GetStreamDescriptorByIndex(streamId, &isSelected, streamDescriptor.put()));

      com_ptr<IMFMediaTypeHandler> mediaTypeHandler;
      check_hresult(streamDescriptor->GetMediaTypeHandler(mediaTypeHandler.put()));

      com_ptr<IMFMediaType> mediaType;
      check_hresult(mediaTypeHandler->GetCurrentMediaType(mediaType.put()));

      guid format;
      check_hresult(mediaType->GetGUID(MF_MT_SUBTYPE, reinterpret_cast<GUID*>(&format)));

      uint32_t width, height;
      check_hresult(MFGetAttributeSize(mediaType.get(), MF_MT_FRAME_SIZE, &width, &height));

      uint32_t fpsNumerator, fpsDenominator;
      check_hresult(MFGetAttributeRatio(mediaType.get(), MF_MT_FRAME_RATE, &fpsNumerator, &fpsDenominator));

      if (format == guid(MFVideoFormat_D16)) //depth
      {
        if (isSelected)
        {
          switch (height)
          {
          case 288:
            configuration.depth_mode = k4a_depth_mode_t::K4A_DEPTH_MODE_NFOV_2X2BINNED;
            break;
          case 576:
            configuration.depth_mode = k4a_depth_mode_t::K4A_DEPTH_MODE_NFOV_UNBINNED;
            break;
          case 512:
            configuration.depth_mode = k4a_depth_mode_t::K4A_DEPTH_MODE_WFOV_2X2BINNED;
            break;
          case 1024:
            configuration.depth_mode = k4a_depth_mode_t::K4A_DEPTH_MODE_WFOV_UNBINNED;
            break;
          default:
            throw hresult_out_of_bounds(L"Invalid depth mode!");
          }
        }
      }
      else //color
      {
        if (isSelected)
        {
          switch (height)
          {
          case 720:
            configuration.color_resolution = k4a_color_resolution_t::K4A_COLOR_RESOLUTION_720P;
            break;
          case 1080:
            configuration.color_resolution = k4a_color_resolution_t::K4A_COLOR_RESOLUTION_1080P;
            break;
          case 1440:
            configuration.color_resolution = k4a_color_resolution_t::K4A_COLOR_RESOLUTION_1440P;
            break;
          case 1536:
            configuration.color_resolution = k4a_color_resolution_t::K4A_COLOR_RESOLUTION_1536P;
            break;
          case 2160:
            configuration.color_resolution = k4a_color_resolution_t::K4A_COLOR_RESOLUTION_2160P;
            break;
          case 3072:
            configuration.color_resolution = k4a_color_resolution_t::K4A_COLOR_RESOLUTION_3072P;
            break;
          default:
            throw hresult_out_of_bounds(L"Invalid color resolution!");
          }

          if (format == guid(MFVideoFormat_RGB32))
          {
            configuration.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
          }
          else if (format == guid(MFVideoFormat_MJPG))
          {
            configuration.color_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
          }
          else if (format == guid(MFVideoFormat_NV12))
          {
            configuration.color_format = K4A_IMAGE_FORMAT_COLOR_NV12;
          }
          else if (format == guid(MFVideoFormat_YUY2))
          {
            configuration.color_format = K4A_IMAGE_FORMAT_COLOR_YUY2;
          }
          else
          {
            throw hresult_out_of_bounds(L"Invalid color format!");
          }
        }
      }

      auto fps = fpsNumerator / fpsDenominator;
      if (isSelected && fps > maxFps)
      {
        maxFps = fps;
      }
    }

    switch (maxFps)
    {
    case 30:
      configuration.camera_fps = k4a_fps_t::K4A_FRAMES_PER_SECOND_30;
      break;
    case 15:
      configuration.camera_fps = k4a_fps_t::K4A_FRAMES_PER_SECOND_15;
      break;
    case 5:
      configuration.camera_fps = k4a_fps_t::K4A_FRAMES_PER_SECOND_5;
      break;
    default:
      throw hresult_out_of_bounds(L"Invalid camera framerate!");
    }

    return configuration;
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
        if (height > 512 && framerate > 15) continue;
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