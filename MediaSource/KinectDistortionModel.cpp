#include "stdafx.h"
#include "KinectDistortionModel.h"
#include <Eigen\Dense>

using namespace DirectX;
using namespace Eigen;

namespace k4u
{
  float ApplyMediaFoundationRadialDistortion(const DirectX::XMFLOAT3& coeffs, float r)
  {
    return 1 +
      coeffs.x * pow(r, 2.f) +
      coeffs.y * pow(r, 4.f) +
      coeffs.z * pow(r, 6.f);
  }
  
  float ApplyKinectRadialDistortion(const DirectX::XMFLOAT3& coeffsA, const DirectX::XMFLOAT3& coeffsB, float r)
  {
    auto a = ApplyMediaFoundationRadialDistortion(coeffsA, r);
    auto b = ApplyMediaFoundationRadialDistortion(coeffsB, r);
    auto bi = b == 0.f ? 1.f : 1.f / b;
    return a * bi;
  }

  DirectX::XMFLOAT3 ConvertKinectRadialDistortionToMediaFoundation(const k4a_calibration_camera_t& calibration)
  {
    const int sampleSize = 100;
    Matrix<float, sampleSize, 1> y1;
    Matrix<float, sampleSize, 3> x;

    auto& params = calibration.intrinsics.parameters.param; 
    XMFLOAT3 coeffsA{ params.k1, params.k2, params.k3 };
    XMFLOAT3 coeffsB{ params.k4, params.k5, params.k6 };

    auto rStep = calibration.metric_radius / (sampleSize - 1);
    auto rCurrent = 0.f;
    for (auto i = 0; i < sampleSize; i++)
    {
      auto r = rCurrent;
      y1(i) = ApplyKinectRadialDistortion(coeffsA, coeffsB, r) - 1;

      auto r2 = r * r;
      auto r4 = r2 * r2;
      auto r6 = r4 * r2;

      x(i, 0) = r2;
      x(i, 1) = r4;
      x(i, 2) = r6;
      rCurrent += rStep;
    }

    auto k = x.fullPivHouseholderQr().solve(y1);
    return { k(0), k(1), k(2) };
  }
}