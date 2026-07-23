#include <Gx/Gx.h>
#include <Tempest/c44matrix.h>
#include <Tempest/c4vector.h>

#include <storm.h>
#include <windows.h>

#ifdef INFINITY
#undef INFINITY
#endif
static float D3dCeil(float f) {
    // TODO: implement
    return 0;
}

static float OglFloor(float f) {
    // TODO: implement
    return 0;
}

static void PixSnap(const CGxCaps& caps, const NTempest::C3Vector& src, NTempest::C3Vector& dst) {
    // TODO: implement
}

void __fastcall GxuXformCreateProjection(float fovyInRadians, float aspect, float minZ, float maxZ, NTempest::C44Matrix &dst) {
  ASSERT(fovyInRadians > 0.0f && fovyInRadians < 3.1415927f);
  ASSERT(aspect > 0.0f);
  ASSERT(minZ < maxZ);

  dst.a1 = 0.0f;
  dst.a2 = 0.0f;
  dst.a3 = 0.0f;
  dst.b0 = 0.0f;
  dst.b2 = 0.0f;
  dst.b3 = 0.0f;
  dst.c0 = 0.0f;
  dst.c1 = 0.0f;
  dst.c3 = 1.0f;
  dst.d0 = 0.0f;
  dst.d1 = 0.0f;
  dst.d3 = 0.0f;

  float halfHeight = tanf(fovyInRadians / NTempest::CMath::sqrt_(aspect * aspect + 1.0f) * 0.5f) * minZ;
  dst.a0 = minZ / (aspect * halfHeight);
  dst.b1 = minZ / halfHeight;

  float depth = maxZ - minZ;
  dst.c2 = (minZ + maxZ) / depth;
  dst.d2 = minZ * maxZ * -2.0f / depth;
}

void __fastcall GxuXformCreateOrtho(float minX, float maxX, float minY, float maxY, float minZ, float maxZ, NTempest::C44Matrix &dst) {
  FATALASSERT(minX != maxX);

  FATALASSERT(minY != maxY);

  FATALASSERT(minZ < maxZ);

  dst.a0 = 2.0f / (maxX - minX);
  dst.b0 = 0.0f;
  dst.c0 = 0.0f;
  dst.d0 = -((minX + maxX) / (maxX - minX));

  dst.a1 = 0.0f;
  dst.b1 = 2.0f / (maxY - minY);
  dst.c1 = 0.0f;
  dst.d1 = -((minY + maxY) / (maxY - minY));

  dst.a2 = 0.0f;
  dst.b2 = 0.0f;
  dst.c2 = 2.0f / (maxZ - minZ);
  dst.d2 = -((minZ + maxZ) / (maxZ - minZ));

  dst.a3 = 0.0f;
  dst.b3 = 0.0f;
  dst.c3 = 0.0f;
  dst.d3 = 1.0f;
}

void __fastcall GxuXformCreateLookAtSgCompat(
    const NTempest::C3Vector &eye,
    const NTempest::C3Vector &center,
    const NTempest::C3Vector &up,
    NTempest::C44Matrix      &dst
) {
  dst = NTempest::C44Matrix();

  NTempest::C3Vector zv = center - eye;
  ASSERT(zv.SquaredMag() >= 0.01f);
  ASSERT(up.SquaredMag() >= 0.01f);
  zv.Normalize();

  NTempest::C3Vector xv = NTempest::C3Vector::Cross(zv, up);
  xv.Normalize();
  NTempest::C3Vector yv = NTempest::C3Vector::Cross(xv, zv);
  yv.Normalize();

  dst.a0 = xv.x;
  dst.a1 = yv.x;
  dst.a2 = zv.x;
  dst.b0 = xv.y;
  dst.b1 = yv.y;
  dst.b2 = zv.y;
  dst.c0 = xv.z;
  dst.c1 = yv.z;
  dst.c2 = zv.z;

  dst.Translate(NTempest::C3Vector(-eye.x, -eye.y, -eye.z));
}

void __fastcall GxuXformCreateLookAtXXX(const NTempest::C3Vector& eye, const NTempest::C3Vector& center, const NTempest::C3Vector& up, NTempest::C44Matrix& dst) {
    // TODO: implement
}

void __fastcall GxuXformCalcFrustumCorners(NTempest::C44Matrix &view, NTempest::C44Matrix &proj, NTempest::C3Vector *corners) {
  NTempest::C44Matrix projInv = proj.Inverse(proj.Determinant());
  NTempest::C44Matrix viewInv = view.Inverse(view.Determinant());
  NTempest::C44Matrix inv = projInv * viewInv;
  float               zMin;
  float               zMax;

  if (NTempest::CMath::fabs_(proj.d3 - 1.0f) < 2.38418579e-7f) {
    zMin = -1.0f;
    zMax = 1.0f;
  } else {
    zMin = -(proj.d2 / (proj.c2 + 1.0f));
    zMax = -(proj.d2 / (proj.c2 - 1.0f));
  }

#define GXU_SET_FRUSTUM_CORNER(i, px, py, pz, pw)                                 \
  corners[i].x = ((px) * inv.a0 + (py) * inv.b0 + (pz) * inv.c0 + (pw) * inv.d0); \
  corners[i].y = ((px) * inv.a1 + (py) * inv.b1 + (pz) * inv.c1 + (pw) * inv.d1); \
  corners[i].z = ((px) * inv.a2 + (py) * inv.b2 + (pz) * inv.c2 + (pw) * inv.d2)

  if (NTempest::CMath::fabs_(proj.d3 - 1.0f) < 2.38418579e-7f) {
    GXU_SET_FRUSTUM_CORNER(0, -1.0f, -1.0f, -1.0f, 1.0f);
    GXU_SET_FRUSTUM_CORNER(1, -1.0f, 1.0f, -1.0f, 1.0f);
    GXU_SET_FRUSTUM_CORNER(2, 1.0f, 1.0f, -1.0f, 1.0f);
    GXU_SET_FRUSTUM_CORNER(3, 1.0f, -1.0f, -1.0f, 1.0f);
    GXU_SET_FRUSTUM_CORNER(4, -1.0f, -1.0f, 1.0f, 1.0f);
    GXU_SET_FRUSTUM_CORNER(5, -1.0f, 1.0f, 1.0f, 1.0f);
    GXU_SET_FRUSTUM_CORNER(6, 1.0f, 1.0f, 1.0f, 1.0f);
    GXU_SET_FRUSTUM_CORNER(7, 1.0f, -1.0f, 1.0f, 1.0f);
  } else {
    GXU_SET_FRUSTUM_CORNER(0, -zMin, -zMin, -zMin, zMin);
    GXU_SET_FRUSTUM_CORNER(1, -zMin, zMin, -zMin, zMin);
    GXU_SET_FRUSTUM_CORNER(2, zMin, zMin, -zMin, zMin);
    GXU_SET_FRUSTUM_CORNER(3, zMin, -zMin, -zMin, zMin);
    GXU_SET_FRUSTUM_CORNER(4, -zMax, -zMax, zMax, zMax);
    GXU_SET_FRUSTUM_CORNER(5, -zMax, zMax, zMax, zMax);
    GXU_SET_FRUSTUM_CORNER(6, zMax, zMax, zMax, zMax);
    GXU_SET_FRUSTUM_CORNER(7, zMax, -zMax, zMax, zMax);
  }

#undef GXU_SET_FRUSTUM_CORNER
}

void __fastcall GxuXformCalcFrustumPlanes(NTempest::C44Matrix &viewProj, NTempest::C4Vector *planes) {
  planes[0] = NTempest::C4Vector(viewProj.a3 + viewProj.a0, viewProj.b3 + viewProj.b0, viewProj.c3 + viewProj.c0, viewProj.d3 + viewProj.d0);
  planes[1] = NTempest::C4Vector(viewProj.a3 - viewProj.a0, viewProj.b3 - viewProj.b0, viewProj.c3 - viewProj.c0, viewProj.d3 - viewProj.d0);
  planes[2] = NTempest::C4Vector(viewProj.a3 + viewProj.a1, viewProj.b3 + viewProj.b1, viewProj.c3 + viewProj.c1, viewProj.d3 + viewProj.d1);
  planes[3] = NTempest::C4Vector(viewProj.a3 - viewProj.a1, viewProj.b3 - viewProj.b1, viewProj.c3 - viewProj.c1, viewProj.d3 - viewProj.d1);
  planes[4] = NTempest::C4Vector(viewProj.a3 + viewProj.a2, viewProj.b3 + viewProj.b2, viewProj.c3 + viewProj.c2, viewProj.d3 + viewProj.d2);
  planes[5] = NTempest::C4Vector(viewProj.a3 - viewProj.a2, viewProj.b3 - viewProj.b2, viewProj.c3 - viewProj.c2, viewProj.d3 - viewProj.d2);

  for (unsigned int i = 0; i < 6; ++i) {
    float mag = NTempest::CMath::sqrt_(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
    if (mag > 0.0f) {
      planes[i].x /= mag;
      planes[i].y /= mag;
      planes[i].z /= mag;
      planes[i].w /= mag;
    }
  }
}

void __fastcall GxuXformCalcFrustumBounds(const NTempest::C44Matrix& view, const NTempest::C44Matrix& proj, NTempest::C3Vector& minBound, NTempest::C3Vector& maxBound) {
    // TODO: implement
}

void __fastcall GxuXformCalc2dScreenCoords(unsigned int count, const NTempest::C3Vector* src, NTempest::C3Vector* dst) {
    // TODO: implement
}

void __fastcall GxuTexScale(const void* srcPixels, EGxTexFormat srcFormat, unsigned int srcW, unsigned int srcH, unsigned int srcStrideInBytes, const void* dstPixels, EGxTexFormat dstFormat, unsigned int dstW, unsigned int dstH, unsigned int dstStrideInBytes) {
    // TODO: implement
}

void __fastcall GxuUpdateSingleColorTexture(
    EGxTexCommand cmd,
    unsigned int  w,
    unsigned int  h,
    unsigned int  d,
    unsigned int  mipLevel,
    void         *userArg,
    unsigned int &texelStrideInBytes,
    const void  *&texels
) {
  static NTempest::CImVector image[64];
  unsigned int               index;

  switch (cmd) {
    case GxTex_Lock:
      for (index = 0; index < 64; ++index) {
        image[index].Set(reinterpret_cast<unsigned long>(userArg));
      }
      break;

    case GxTex_Latch:
      texelStrideInBytes = 4 * w;
      texels = image;
      break;
  }
}

int __fastcall GxuTestRayAndSphere(const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayDirection, const NTempest::C3Vector& sphereCenter, float sphereRadius, float& distance) {
    // TODO: implement
    return 0;
}

int __fastcall GxuTestSphereAndFrustumPlanes(NTempest::C3Vector &center, float radius, NTempest::C4Vector *planes, unsigned int numPlanes) {
  for (unsigned int i = 0; i < numPlanes; ++i) {
    if (planes[i].x * center.x + planes[i].y * center.y + planes[i].z * center.z + planes[i].w < -radius) {
      return 0;
    }
  }
  return 1;
}

int __fastcall GxuTestRayAndTriangle(
    const NTempest::C3Vector &rayStart,
    const NTempest::C3Vector &rayDirection,
    const NTempest::C3Vector &v0,
    const NTempest::C3Vector &v1,
    const NTempest::C3Vector &v2,
    float                    &distance
) {
  using NTempest::IsUnitVector;

  distance = INFINITY;

  FATALASSERT(IsUnitVector(rayDirection));

  NTempest::C3Vector e1 = v1 - v0;
  NTempest::C3Vector e2 = v2 - v0;
  NTempest::C3Vector p = NTempest::C3Vector::Cross(rayDirection, e2);
  float              f = NTempest::C3Vector::Dot(e1, p);

  if (NTempest::CMath::fabs_(f) < 2.38418579e-7f) {
    return 0;
  }

  f = 1.0f / f;
  NTempest::C3Vector s = rayStart - v0;
  float              u = NTempest::C3Vector::Dot(s, p) * f;
  if (u < 0.0f || u > 1.0f) {
    return 0;
  }

  NTempest::C3Vector q = NTempest::C3Vector::Cross(s, e1);
  float              v = NTempest::C3Vector::Dot(rayDirection, q) * f;
  if (v < 0.0f || u + v > 1.0f) {
    return 0;
  }

  distance = NTempest::C3Vector::Dot(e2, q) * f;
  return 1;
}
int __fastcall GxuTestRayAndMesh(const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayDirection, const NTempest::C34Matrix* modelToWorldMatrices, unsigned int matrixCount, unsigned int posCount, const NTempest::C3Vector* pos, unsigned int posStride, unsigned int boneCount, const unsigned char* bone, unsigned int boneStride, EGxPrim primType, unsigned int indexCount, const unsigned short* indices, float& distance, unsigned int& primIntersected) {
    // TODO: implement
    return 0;
}

int __fastcall GxuTestRayAndRigidMeshInModelSpace(const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayDirection, unsigned int posCount, const NTempest::C3Vector* pos, EGxPrim primType, unsigned int indexCount, const unsigned short* indices, float& distance, unsigned int& primIntersected) {
    // TODO: implement
    return 0;
}

unsigned int __fastcall GxuClipCalcCode(const NTempest::C44Matrix& viewProj, const NTempest::C3Vector& pos) {
    // TODO: implement
    return 0;
}

void __fastcall GxuSnapTexelsToPixels(const NTempest::C3Vector* pos, NTempest::C2Vector* tex, unsigned int texW, unsigned int texH) {
    // TODO: implement
}
