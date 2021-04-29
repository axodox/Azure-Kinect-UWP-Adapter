#include "stdafx.h"

#if !defined(_IKsControl_)
#define _IKsControl_
interface DECLSPEC_UUID("28F54685-06FD-11D2-B27A-00A0C9223196") IKsControl;
#undef INTERFACE
#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl, IUnknown)
{
  STDMETHOD(KsProperty)(
    THIS_
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT LPVOID PropertyData,
    IN ULONG DataLength,
    OUT ULONG * BytesReturned
    ) PURE;
  STDMETHOD(KsMethod)(
    THIS_
    IN PKSMETHOD Method,
    IN ULONG MethodLength,
    IN OUT LPVOID MethodData,
    IN ULONG DataLength,
    OUT ULONG * BytesReturned
    ) PURE;
  STDMETHOD(KsEvent)(
    THIS_
    IN PKSEVENT Event OPTIONAL,
    IN ULONG EventLength,
    IN OUT LPVOID EventData,
    IN ULONG DataLength,
    OUT ULONG * BytesReturned
    ) PURE;
};
#endif  // _IKsControl_