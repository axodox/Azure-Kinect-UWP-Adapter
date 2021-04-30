#include "stdafx.h"
#include "KinectMediaSource.h"

using namespace k4u;
using namespace std;
using namespace winrt;

//HRESULT __stdcall DllGetActivationFactory(void* activatibleClassId, void** factory)
STDAPI DllGetActivationFactory(_In_ HSTRING activatibleClassId, _COM_Outptr_ IActivationFactory** factory)
{
  /*if (!factory) return E_POINTER;

  wstring_view const name{ *reinterpret_cast<hstring*>(&activatibleClassId) };
  if (name == L"KinectMediaSource" || name == L"CustomCameraSource")
  {
    *factory = make<KinectMediaSourceClassFactory>().as<IActivationFactory>().detach();
    return S_OK;
  }*/

  return CLASS_E_CLASSNOTAVAILABLE;
}

//HRESULT __stdcall DllGetClassObject(GUID const& classId, GUID const& interfaceId, void** result)
STDAPI DllGetClassObject(REFCLSID classId, REFIID interfaceId, _COM_Outptr_ void** result)
{
  try
  {
    *result = nullptr;

    if (classId == __uuidof(KinectMediaSource))
    {
      return winrt::make<KinectMediaSourceClassFactory>()->QueryInterface(interfaceId, result);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
  }
  catch (...)
  {
    return to_hresult();
  }
}

//HRESULT __stdcall DllCanUnloadNow()
STDAPI DllCanUnloadNow()
{
  if (get_module_lock())
  {
    return S_FALSE;
  }

  clear_factory_cache();
  return S_OK;
}

//BOOL __stdcall DllMain(HINSTANCE instance, DWORD reason, void*)
STDAPI_(BOOL) DllMain(_In_opt_ HINSTANCE instance, DWORD reason, _In_opt_ void*)
{
  if (reason != DLL_PROCESS_ATTACH) return TRUE;

  {
    ofstream s("C:\\kinect.log");
    s << GetCurrentProcessId();
  }

  /*while (!IsDebuggerPresent()) Sleep(1000);
  DebugBreak();*/

  DisableThreadLibraryCalls(instance);
  return TRUE;
}
