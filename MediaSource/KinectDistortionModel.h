#pragma once
#include "stdafx.h"

namespace k4u
{
  float ApplyMediaFoundationRadialDistortion(const DirectX::XMFLOAT3& coeffs, float r);
  float ApplyKinectRadialDistortion(const DirectX::XMFLOAT3& coeffsA, const DirectX::XMFLOAT3& coeffsB, float r);
  DirectX::XMFLOAT3 ConvertKinectRadialDistortionToMediaFoundation(const k4a_calibration_camera_t& calibration);
}