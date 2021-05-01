#pragma once

// Std header files:
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <fstream>

// Windows Header Files:
#define NOMINMAX
#include <windows.h>
#include <propvarutil.h>
//#include <mfstd.h> // Must be included before <initguid.h>, or else DirectDraw GUIDs will be defined twice. See the comment in <uuids.h>.
#include <ole2.h>
#include <initguid.h>
#include <ks.h>
#include <ksmedia.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <nserror.h>
#include <winmeta.h>
#include <d3d9types.h>
#include <dxgiformat.h>
#include <activation.h>
#include <hstring.h>
#include <winrt/base.h>

//Kinect header files
#include <k4a/k4a.h>
#pragma comment (lib, "k4a.lib")