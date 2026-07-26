#include <Gx/Gx.h>
#include <Tempest/caabox.h>
#include <Tempest/c33matrix.h>
#include <Tempest/c34matrix.h>
#include <Tempest/c44matrix.h>
#include <Tempest/c4vector.h>

#include <storm.h>
#include <stpl.h>
#include <windows.h>

#ifdef INFINITY
#undef INFINITY
#endif
static const float PI = 3.14159265358979323846f;

static float D3dCeil(float f) {
  return static_cast<float>(ceil(floor(f * 16.0f + 0.5f) * 0.0625f));
}

static float OglFloor(float f) {
  return static_cast<float>(ceil(floor(f * 16.0f + 0.5f) * 0.0625f));
}

static void PixSnap(const CGxCaps& caps, const NTempest::C3Vector& src, NTempest::C3Vector& dst) {
  if (caps.m_pixelCenterOnEdge && caps.m_texelCenterOnEdge) {
    dst.Set(OglFloor(src.x), OglFloor(src.y), 1.0f);
  } else if (!caps.m_pixelCenterOnEdge && caps.m_texelCenterOnEdge) {
    dst.Set(D3dCeil(src.x), D3dCeil(src.y), 1.0f);
  } else {
    ASSERT(0);
  }
}

void __fastcall GxuXformCreateProjection(float fovyInRadians, float aspect, float minZ, float maxZ, NTempest::C44Matrix &dst) {
  ASSERT(fovyInRadians > 0.0f && fovyInRadians < PI);
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

void __fastcall GxuXformCreateOrtho(const NTempest::CAaBox &bounds, NTempest::C44Matrix &dst) {
  GxuXformCreateOrtho(bounds.b.x, bounds.t.x, bounds.b.y, bounds.t.y, bounds.b.z, bounds.t.z, dst);
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
  dst = NTempest::C44Matrix();

  NTempest::C3Vector zv = center - eye;
  FATALASSERT(zv.SquaredMag() >= 0.01f);
  FATALASSERT(up.SquaredMag() >= 0.01f);
  zv.Normalize();

  NTempest::C3Vector xv = NTempest::C3Vector::Cross(up, zv);
  xv.Normalize();
  NTempest::C3Vector yv = NTempest::C3Vector::Cross(zv, xv);
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

void __fastcall GxuXformCalcFrustumCorners(const NTempest::C44Matrix &view, const NTempest::C44Matrix &proj, NTempest::C3Vector *corners) {
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

void __fastcall GxuXformCalcFrustumPlanes(const NTempest::C44Matrix &viewProj, NTempest::C4Vector *planes) {
  planes[0] = NTempest::C4Vector(viewProj.a0 - viewProj.a3, viewProj.b0 - viewProj.b3, viewProj.c0 - viewProj.c3, viewProj.d0 - viewProj.d3);
  planes[1] = NTempest::C4Vector(-viewProj.a0 - viewProj.a3, -viewProj.b0 - viewProj.b3, -viewProj.c0 - viewProj.c3, -viewProj.d0 - viewProj.d3);
  planes[2] = NTempest::C4Vector(viewProj.a1 - viewProj.a3, viewProj.b1 - viewProj.b3, viewProj.c1 - viewProj.c3, viewProj.d1 - viewProj.d3);
  planes[3] = NTempest::C4Vector(-viewProj.a1 - viewProj.a3, -viewProj.b1 - viewProj.b3, -viewProj.c1 - viewProj.c3, -viewProj.d1 - viewProj.d3);
  planes[4] = NTempest::C4Vector(viewProj.a2 - viewProj.a3, viewProj.b2 - viewProj.b3, viewProj.c2 - viewProj.c3, viewProj.d2 - viewProj.d3);
  planes[5] = NTempest::C4Vector(-viewProj.a2 - viewProj.a3, -viewProj.b2 - viewProj.b3, -viewProj.c2 - viewProj.c3, -viewProj.d2 - viewProj.d3);

  for (unsigned int i = 0; i < 6; ++i) {
    float invMag =
        1.0f / NTempest::CMath::sqrt_(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
    planes[i].x *= invMag;
    planes[i].y *= invMag;
    planes[i].z *= invMag;
    planes[i].w *= invMag;
  }
}

void __fastcall GxuXformCalcFrustumBounds(const NTempest::C44Matrix& view, const NTempest::C44Matrix& proj, NTempest::C3Vector& minBound, NTempest::C3Vector& maxBound) {
  NTempest::C3Vector corners[8];
  GxuXformCalcFrustumCorners(view, proj, corners);
  minBound = corners[0];
  maxBound = corners[0];
  for (unsigned int i = 0; i < 8; ++i) {
    minBound = NTempest::C3Vector::Min(minBound, corners[i]);
    maxBound = NTempest::C3Vector::Max(maxBound, corners[i]);
  }
}

void __fastcall GxuXformCalc2dScreenCoords(unsigned int count, const NTempest::C3Vector* src, NTempest::C3Vector* dst) {
  NTempest::CRect winRect;
  NTempest::C3Vector vpMin;
  NTempest::C3Vector vpMax;
  NTempest::C44Matrix viewProj;
  GxCapsWindowSize(winRect);
  GxXformViewport(vpMin.x, vpMax.x, vpMin.y, vpMax.y, vpMin.z, vpMax.z);
  GxXformViewProj(viewProj);

  float vpWidth = vpMax.x - vpMin.x;
  float vpHeight = vpMax.y - vpMin.y;
  for (unsigned int i = 0; i < count; ++i) {
    dst[i].x = viewProj.a0 * src[i].x + viewProj.b0 * src[i].y + viewProj.d0;
    dst[i].y = viewProj.a1 * src[i].x + viewProj.b1 * src[i].y + viewProj.d1;
    dst[i].z = 1.0f;
    dst[i].x = ((dst[i].x + 1.0f) * vpWidth * 0.5f + vpMin.x) * winRect.r;
    dst[i].y = winRect.b - ((dst[i].y + 1.0f) * vpHeight * 0.5f + vpMin.y) * winRect.b;
    dst[i].z = 1.0f;
  }
}

void __fastcall GxuTexScale(const void* srcPixels, EGxTexFormat srcFormat, unsigned int srcW, unsigned int srcH, unsigned int srcStrideInBytes, const void* dstPixels, EGxTexFormat dstFormat, unsigned int dstW, unsigned int dstH, unsigned int dstStrideInBytes) {
  ASSERT(srcFormat == dstFormat);
  FATALASSERT(srcPixels);
  FATALASSERT(srcFormat < GxTexFormats_Last);
  FATALASSERT(dstPixels);
  FATALASSERT(dstFormat < GxTexFormats_Last);

  const unsigned char *src = static_cast<const unsigned char *>(srcPixels);
  unsigned char       *dst = const_cast<unsigned char *>(static_cast<const unsigned char *>(dstPixels));
  unsigned int         stepX = (srcW << 16) / dstW;
  unsigned int         stepY = (srcH << 16) / dstH;
  unsigned int         srcY = 0;
  unsigned int         pixelSize = srcFormat == GxTex_Argb8888 ? 4 : 2;
  for (unsigned int y = 0; y < dstH; ++y) {
    unsigned int srcX = 0;
    for (unsigned int x = 0; x < dstW; ++x) {
      if (pixelSize == 4) {
        *reinterpret_cast<unsigned int *>(dst + x * 4) =
            *reinterpret_cast<const unsigned int *>(src + HIWORD(srcY) * srcStrideInBytes + HIWORD(srcX) * 4);
      } else {
        *reinterpret_cast<unsigned short *>(dst + x * 2) =
            *reinterpret_cast<const unsigned short *>(src + HIWORD(srcY) * srcStrideInBytes + HIWORD(srcX) * 2);
      }
      srcX += stepX;
    }
    srcY += stepY;
    dst += dstStrideInBytes;
  }
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
  using NTempest::IsUnitVector;

  distance = INFINITY;
  FATALASSERT(IsUnitVector(rayDirection));
  float centerDistance = NTempest::C3Vector::Dot(sphereCenter - rayStart, rayDirection);
  if (centerDistance < -sphereRadius) {
    return 0;
  }
  NTempest::C3Vector closest = rayStart + rayDirection * centerDistance - sphereCenter;
  if (closest.SquaredMag() > sphereRadius * sphereRadius) {
    return 0;
  }
  distance = centerDistance;
  return 1;
}

int __fastcall GxuTestSphereAndFrustumPlanes(const NTempest::C3Vector &center, float radius, const NTempest::C4Vector *planes) {
  for (unsigned int i = 0; i < 6; ++i) {
    if (planes[i].x * center.x + planes[i].y * center.y + planes[i].z * center.z + planes[i].w > radius) {
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
  using NTempest::IsUnitVector;

  distance = INFINITY;
  primIntersected = 0;

  FATALASSERT(IsUnitVector(rayDirection));
  FATALASSERT(posCount);
  FATALASSERT(pos);
  FATALASSERT(primType == GxPrim_Triangles || primType == GxPrim_TriangleStrip || primType == GxPrim_TriangleFan);
  FATALASSERT(indexCount >= 3);

  static TSFixedArray_<NTempest::C3Vector, 'GxuT', 759> tmpVtx;

  NTempest::C34Matrix identity;
  if (!modelToWorldMatrices) {
    modelToWorldMatrices = &identity;
  }

  tmpVtx.SetCount(posCount);
  const unsigned char *posBytes = reinterpret_cast<const unsigned char *>(pos);
  const unsigned char *boneIndex = bone;
  for (unsigned int vndx = 0; vndx < posCount; ++vndx) {
    unsigned int id = boneIndex ? *boneIndex : 0;
    FATALASSERT(boneStride ? vndx < boneCount : 1);
    FATALASSERT(id < matrixCount);
    tmpVtx[vndx] = *reinterpret_cast<const NTempest::C3Vector *>(posBytes) * modelToWorldMatrices[id];
    posBytes += posStride;
    if (boneIndex) {
      boneIndex += boneStride;
    }
  }

  float currDistance;
  switch (primType) {
    case GxPrim_Triangles: {
      for (unsigned int i = 0, primitive = 0; i < indexCount; i += 3, ++primitive) {
        if (GxuTestRayAndTriangle(
                rayStart, rayDirection, tmpVtx[indices[i]], tmpVtx[indices[i + 1]], tmpVtx[indices[i + 2]], currDistance
            ) &&
            currDistance >= 0.0f && currDistance < distance)
        {
          distance = currDistance;
          primIntersected = primitive;
        }
      }
      break;
    }

    case GxPrim_TriangleStrip: {
      for (unsigned int i = 2; i < indexCount; ++i) {
        if (GxuTestRayAndTriangle(
                rayStart, rayDirection, tmpVtx[indices[i - 2]], tmpVtx[indices[i - 1]], tmpVtx[indices[i]], currDistance
            ) &&
            currDistance >= 0.0f && currDistance < distance)
        {
          distance = currDistance;
          primIntersected = i - 2;
        }
      }
      break;
    }

    case GxPrim_TriangleFan: {
      for (unsigned int i = 2; i < indexCount; ++i) {
        if (GxuTestRayAndTriangle(
                rayStart, rayDirection, tmpVtx[indices[0]], tmpVtx[indices[i - 1]], tmpVtx[indices[i]], currDistance
            ) &&
            currDistance >= 0.0f && currDistance < distance)
        {
          distance = currDistance;
          primIntersected = i - 2;
        }
      }
      break;
    }
  }

  return distance != INFINITY;
}

int __fastcall GxuTestRayAndRigidMeshInModelSpace(const NTempest::C3Vector& rayStart, const NTempest::C3Vector& rayDirection, unsigned int posCount, const NTempest::C3Vector* pos, EGxPrim primType, unsigned int indexCount, const unsigned short* indices, float& distance, unsigned int& primIntersected) {
  using NTempest::IsUnitVector;

  distance = INFINITY;
  primIntersected = 0;
  FATALASSERT(IsUnitVector(rayDirection));
  FATALASSERT(posCount);
  FATALASSERT(pos);
  FATALASSERT(primType == GxPrim_Triangles || primType == GxPrim_TriangleStrip || primType == GxPrim_TriangleFan);
  FATALASSERT(indexCount >= 3);

  float currDistance;
  switch (primType) {
    case GxPrim_Triangles: {
      for (unsigned int i = 0, primitive = 0; i < indexCount; i += 3, ++primitive) {
        if (GxuTestRayAndTriangle(rayStart, rayDirection, pos[indices[i]], pos[indices[i + 1]], pos[indices[i + 2]], currDistance) &&
            currDistance < distance)
        {
          distance = currDistance;
          primIntersected = primitive;
        }
      }
      break;
    }
    case GxPrim_TriangleStrip: {
      for (unsigned int i = 2; i < indexCount; ++i) {
        if (GxuTestRayAndTriangle(rayStart, rayDirection, pos[indices[i - 2]], pos[indices[i - 1]], pos[indices[i]], currDistance) &&
            currDistance < distance)
        {
          distance = currDistance;
          primIntersected = i - 2;
        }
      }
      break;
    }
    case GxPrim_TriangleFan: {
      for (unsigned int i = 2; i < indexCount; ++i) {
        if (GxuTestRayAndTriangle(rayStart, rayDirection, pos[indices[0]], pos[indices[i - 1]], pos[indices[i]], currDistance) &&
            currDistance < distance)
        {
          distance = currDistance;
          primIntersected = i - 2;
        }
      }
      break;
    }
  }
  return distance != INFINITY;
}

unsigned int __fastcall GxuClipCalcCode(const NTempest::C44Matrix& viewProj, const NTempest::C3Vector& pos) {
  NTempest::C4Vector clipVert(pos.x, pos.y, pos.z, 1.0f);
  clipVert = clipVert * viewProj;
  float cc[6] = {
      clipVert.w - clipVert.x,
      clipVert.x + clipVert.w,
      clipVert.w - clipVert.y,
      clipVert.y + clipVert.w,
      clipVert.w - clipVert.z,
      clipVert.z + clipVert.w
  };
  unsigned int code = 0;
  const unsigned int *bits = reinterpret_cast<const unsigned int *>(cc);
  for (unsigned int i = 0; i < 6; ++i) {
    code |= (bits[i] >> 31) << i;
  }
  return code;
}

void __fastcall GxuSnapTexelsToPixels(const NTempest::C3Vector* pos, NTempest::C2Vector* tex, unsigned int texW, unsigned int texH) {
  unsigned int i;
  NTempest::C3Vector posScr[4];
  NTempest::C3Vector iposScr[4];
  NTempest::C3Vector texScr[4];
  NTempest::C3Vector iposCntr(0.0f);
  const CGxCaps      caps = GxCaps();

  GxuXformCalc2dScreenCoords(4, pos, posScr);
  texScr[0].Set(tex[0].x, tex[0].y, 0.0f);
  texScr[1] = texScr[0];
  for (i = 0; i < 4; ++i) {
    PixSnap(caps, posScr[i], iposScr[i]);
    iposCntr += iposScr[i] * 0.25f;
    if (tex[i].x < texScr[0].x) texScr[0].x = tex[i].x;
    if (tex[i].y < texScr[0].y) texScr[0].y = tex[i].y;
    if (tex[i].x > texScr[1].x) texScr[1].x = tex[i].x;
    if (tex[i].y > texScr[1].y) texScr[1].y = tex[i].y;
  }

  float texArea = (texScr[1].x - texScr[0].x) * (texScr[1].y - texScr[0].y) * static_cast<float>(texW) * static_cast<float>(texH);
  for (i = 0; i < 4; ++i) {
    if (iposScr[i].x > iposCntr.x) iposScr[i].x -= 1.0f;
    if (iposScr[i].y > iposCntr.y) iposScr[i].y -= 1.0f;
  }

  NTempest::C3Vector normal = NTempest::C3Vector::Cross(iposScr[2] - iposScr[1], iposScr[0] - iposScr[1]);
  float pixArea = normal.Mag();
  if (pixArea < 1.0f) {
    return;
  }

  pixArea *= 3.0f;
  unsigned int mip = 0;
  while (pixArea <= texArea) {
    texArea *= 0.5f;
    ++mip;
  }

  unsigned int mipW = texW >> mip;
  unsigned int mipH = texH >> mip;
  if (!mipW || !mipH) {
    return;
  }

  float texCenterX = (texScr[1].x + texScr[0].x) * static_cast<float>(mipW) * 0.5f;
  float texCenterY = (texScr[1].y + texScr[0].y) * static_cast<float>(mipH) * 0.5f;
  for (i = 0; i < 4; ++i) {
    texScr[i].x = tex[i].x * static_cast<float>(mipW);
    texScr[i].y = tex[i].y * static_cast<float>(mipH);
    texScr[i].z = 1.0f;
    texScr[i].x += texScr[i].x >= texCenterX ? -0.5f : 0.5f;
    texScr[i].y += texScr[i].y >= texCenterY ? -0.5f : 0.5f;
  }

  NTempest::C33Matrix iposM33(
      iposScr[0].x, iposScr[0].y, 1.0f,
      iposScr[1].x, iposScr[1].y, 1.0f,
      iposScr[2].x, iposScr[2].y, 1.0f
  );
  NTempest::C33Matrix texM33(
      texScr[0].x, texScr[0].y, 1.0f,
      texScr[1].x, texScr[1].y, 1.0f,
      texScr[2].x, texScr[2].y, 1.0f
  );
  NTempest::C33Matrix fromPixToTexM33 = iposM33.Inverse(iposM33.Determinant()) * texM33;
  for (i = 0; i < 4; ++i) {
    NTempest::C3Vector snapped = fromPixToTexM33 * posScr[i];
    tex[i].x = snapped.x / static_cast<float>(mipW);
    tex[i].y = snapped.y / static_cast<float>(mipH);
  }
}
