#include "stdafx.h"
#include "KinectMediaTypeCollection.h"

using namespace std;
using namespace winrt;

namespace k4u
{
  void KinectMediaTypeCollection::AddType(const winrt::guid& format, uint32_t width, uint32_t height, uint32_t framesPerSecond)
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
    
    _types.push_back(move(type));
  }

  void KinectMediaTypeCollection::AddColorTypes()
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
          AddType(format, width, height, framerate);
        }

        if (format != guid(MFVideoFormat_RGB32) && 
          format != guid(MFVideoFormat_MJPG))
        {
          break;
        }
      }
    }    
  }

  void KinectMediaTypeCollection::AddDepthTypes()
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
        AddType(MFVideoFormat_D16, width, height, framerate);
      }
    }
  }

  std::vector<IMFMediaType*> KinectMediaTypeCollection::GetTypes()
  {
    vector<IMFMediaType*> results;
    for (auto& type : _types)
    {
      results.push_back(type.get());
    }
    return results;
  }
}