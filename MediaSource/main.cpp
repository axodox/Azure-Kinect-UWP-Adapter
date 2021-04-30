#include "KinectMediaSource.h"

using namespace winrt;
using namespace k4u;

int main()
{
  auto factory = make_self<KinectMediaSourceClassFactory>();

  com_ptr<IMFMediaSource> source;
  check_hresult(factory->CreateInstance(nullptr, __uuidof(IMFMediaSource), source.put_void()));

  com_ptr<IMFPresentationDescriptor> presentation;
  check_hresult(source->CreatePresentationDescriptor(presentation.put()));

  DWORD streamCount;
  check_hresult(presentation->GetStreamDescriptorCount(&streamCount));

  for (auto streamIndex = 0u; streamIndex < streamCount; streamIndex++)
  {
    int isSelected;
    com_ptr<IMFStreamDescriptor> streamDesc;
    check_hresult(presentation->GetStreamDescriptorByIndex(streamIndex, &isSelected, streamDesc.put()));

    com_ptr<IMFMediaTypeHandler> typeHandler;
    check_hresult(streamDesc->GetMediaTypeHandler(typeHandler.put()));

    DWORD typeCount;
    check_hresult(typeHandler->GetMediaTypeCount(&typeCount));

    printf("%d\n", typeCount);
  }

  return 0;
}