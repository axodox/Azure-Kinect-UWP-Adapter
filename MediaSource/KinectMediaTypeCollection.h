#pragma once
#include "stdafx.h"

namespace k4u
{
  class KinectMediaTypeCollection
  {
  private:
    std::vector<winrt::com_ptr<IMFMediaType>> _types;

  public:
    void AddType(const winrt::guid& format, uint32_t width, uint32_t height, uint32_t framesPerSecond);

    void AddColorTypes();
    void AddDepthTypes();

    std::vector<IMFMediaType*> GetTypes();
  };
}