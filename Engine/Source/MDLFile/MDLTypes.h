#pragma once

#include "Base/Color.h"
#include "Tempest/caabox.h"
#include "Tempest/c2vector.h"
#include "Tempest/c4quaternion.h"

#include <stpl.h>

#ifndef MDL_COMMON_TYPES_DEFINED
#define MDL_COMMON_TYPES_DEFINED

template <unsigned int Length>
struct CMdlString {
  operator char *() {
    return m_string;
  }

  operator const char *() const {
    return m_string;
  }

  char &operator[](unsigned int index) {
    return m_string[index];
  }

  char operator[](unsigned int index) const {
    return m_string[index];
  }

 private:
  char m_string[Length];
};

struct CMdlBounds {
  NTempest::CAaBox extent;
  float            radius;
};

#endif

struct MDLTEXTURESECTION {
  unsigned int    replaceableId;
  CMdlString<260> image;
  unsigned int    flags;
};

#ifndef MDL_TRACK_TYPE_DEFINED
#define MDL_TRACK_TYPE_DEFINED
enum MDLTRACKTYPE {
  TRACK_DONT_INTERP = 0,
  TRACK_LINEAR = 1,
  TRACK_HERMITE = 2,
  TRACK_BEZIER = 3,
  NUM_TRACK_TYPES = 4
};
#endif

template <class T>
struct MDLKEYFRAME {
  int time;
  T   value;
  T   inTan;
  T   outTan;
};

template <class T>
struct MDLKEYTRACK {
  TSGrowableArray<MDLKEYFRAME<T> > keys;
  MDLTRACKTYPE                     type;
  unsigned int                     globalSeqId;
};

struct MDLINTKEY {
  unsigned int time;
  unsigned int value;
};

template <class T>
struct MDLSIMPLEKEYTRACK {
  TSGrowableArray<T> keys;
  unsigned int       globalSeqId;
};

enum MDLTEXOP {
  TEXOP_LOAD = 0,
  TEXOP_TRANSPARENT = 1,
  TEXOP_BLEND = 2,
  TEXOP_ADD = 3,
  TEXOP_ADD_ALPHA = 4,
  TEXOP_MODULATE = 5,
  TEXOP_MODULATE2X = 6,
  NUMTEXOPS = 7
};

struct MDLTEXLAYER {
  MDLTEXOP                     blendMode;
  unsigned int                 flags;
  unsigned int                 textureId;
  MDLSIMPLEKEYTRACK<MDLINTKEY> flipKeys;
  unsigned int                 transformId;
  unsigned int                 coordId;
  MDLKEYTRACK<float>           alphaKeys;
  float                        staticAlpha;
};

struct MDLMATERIALSECTION {
  TSGrowableArray<MDLTEXLAYER> texLayers;
  int                          priorityPlane;
};

struct MDLPRIMITIVES {
  void ReserveSpace(unsigned int numPrimitives, unsigned int numVertices) {
    types.Reserve(numPrimitives);
    counts.Reserve(numPrimitives);
    vertices.Reserve(numVertices);
  }

  void SetCount(unsigned int numPrimitives, unsigned int numVertices) {
    types.SetCount(numPrimitives);
    counts.SetCount(numPrimitives);
    vertices.SetCount(numVertices);
  }

  TSGrowableArray<unsigned char>  types;
  TSGrowableArray<unsigned int>   counts;
  TSGrowableArray<unsigned short> vertices;
};

struct MDLGEOSETSECTION {
  TSGrowableArray<NTempest::C3Vector>                   vertices;
  TSGrowableArray<NTempest::C3Vector>                   normals;
  TSGrowableArray<TSGrowableArray<NTempest::C2Vector> > texCoords;
  TSGrowableArray<unsigned char>                        vertGroupIndices;
  MDLPRIMITIVES                                         primitives;
  TSGrowableArray<unsigned int>                         groupMatrixCounts;
  TSGrowableArray<unsigned int>                         matrices;
  TSGrowableArray<unsigned int>                         boneIndices;
  TSGrowableArray<unsigned int>                         boneWeights;
  unsigned int                                          materialId;
  CMdlBounds                                            bounds;
  TSGrowableArray<CMdlBounds>                           seqBounds;
  unsigned int                                          selectionGroup;
  unsigned int                                          flags;
};

struct MDLGEOSETANIMSECTION {
  MDLKEYTRACK<float>   alphaKeys;
  float                staticAlpha;
  unsigned int         flags;
  MDLKEYTRACK<C3Color> colorKeys;
  C3Color              staticColor;
  unsigned int         geosetId;
};

struct MDLGENOBJECT {
  CMdlString<80>                      name;
  unsigned int                        objectId;
  unsigned int                        parentId;
  unsigned int                        flags;
  MDLKEYTRACK<NTempest::C3Vector>     transkeys;
  MDLKEYTRACK<NTempest::C4Quaternion> rotkeys;
  MDLKEYTRACK<NTempest::C3Vector>     scalekeys;
};

struct MDLATTACHMENTSECTION : public MDLGENOBJECT {
  CMdlString<260>    path;
  MDLKEYTRACK<float> visibilityKeys;
  unsigned int       attachmentId;
};

struct MDLHEADERSECTION {
  CMdlString<257> userName;
  CMdlString<260> sourceFilename;
};

struct MDLMODELSECTION {
  CMdlString<80>  name;
  CMdlString<260> animationFile;
  unsigned int    geosetCount;
  unsigned int    geosetAnimCount;
  unsigned int    boneCount;
  unsigned int    lightCount;
  unsigned int    helperCount;
  unsigned int    attachmentCount;
  unsigned int    particleCount;
  unsigned int    particle2Count;
  unsigned int    ribbonCount;
  unsigned int    eventCount;
  CMdlBounds      bounds;
  unsigned int    blendTime;
  unsigned int    flags;
};

struct MDLCOLLISION {
  TSGrowableArray<NTempest::C3Vector> vertices;
  TSGrowableArray<unsigned short>     triIndices;
  TSGrowableArray<NTempest::C3Vector> facetNormals;
};

namespace NTempest {
#ifndef MDL_CIRANGE_DEFINED
#define MDL_CIRANGE_DEFINED
struct CiRange {
  int l;
  int h;
};
#endif
}

struct MDLSEQUENCESSECTION {
  CMdlString<80>    name;
  NTempest::CiRange time;
  float             movespeed;
  unsigned int      flags;
  CMdlBounds        bounds;
  float             frequency;
  NTempest::CiRange replay;
  unsigned int      blendTime;
};
struct MDLGLOBALSEQSECTION;
struct MDLTEXANIMSECTION {
  MDLKEYTRACK<NTempest::C3Vector>     transkeys;
  MDLKEYTRACK<NTempest::C4Quaternion> rotkeys;
  MDLKEYTRACK<NTempest::C3Vector>     scalekeys;
};
struct MDLBONESECTION : public MDLGENOBJECT {
  unsigned int geosetId;
  unsigned int geosetAnimId;
};
struct MDLLIGHTSECTION;
struct MDLPARTICLEEMITTER;
struct MDLTARGETSECTION {
  NTempest::C3Vector                  pivot;
  MDLKEYTRACK<NTempest::C3Vector>     transkeys;
};
#pragma once

struct MDLCAMERASECTION {
  CMdlString<80>                      name;
  NTempest::C3Vector                  pivot;
  float                               fieldOfView;
  float                               farClip;
  float                               nearClip;
  MDLKEYTRACK<NTempest::C3Vector>     transkeys;
  MDLKEYTRACK<float>                  rollkeys;
  MDLTARGETSECTION                    target;
  MDLKEYTRACK<float>                  visibilityKeys;
};
struct MDLEVENTSECTION;
struct MDLPARTICLEEMITTER2;
enum GEOM_SHAPE {
  SHAPE_BOX = 0,
  SHAPE_CYLINDER = 1,
  SHAPE_SPHERE = 2,
  SHAPE_PLANE = 3,
  NUM_SHAPES = 4
};

struct MDLVECTOR3 {
  float x;
  float y;
  float z;
};

struct MDLBOX {
  MDLVECTOR3 minimum;
  MDLVECTOR3 maximum;
};

struct MDLCYLINDER {
  MDLVECTOR3 base;
  float      height;
  float      radius;
};

struct MDLSPHERE {
  MDLVECTOR3 center;
  float      radius;
};

struct MDLPLANE {
  float length;
  float width;
};

struct MDLHITTESTSHAPE : public MDLGENOBJECT {
  GEOM_SHAPE type;
  union {
    MDLBOX      box;
    MDLCYLINDER cylinder;
    MDLSPHERE   sphere;
    MDLPLANE    plane;
  } shape;
};
struct MDLRIBBONEMITTER;

struct MDLBASE {
  MDLHEADERSECTION                      header;
  MDLMODELSECTION                       model;
  unsigned int                          version;
  TSGrowableArray<MDLSEQUENCESSECTION>  sequences;
  TSGrowableArray<MDLGLOBALSEQSECTION>  globalSeqs;
  TSGrowableArray<MDLMATERIALSECTION>   materials;
  TSGrowableArray<MDLTEXTURESECTION>    textures;
  TSGrowableArray<MDLTEXANIMSECTION>    textureanims;
  TSGrowableArray<MDLGEOSETSECTION>     geosets;
  TSGrowableArray<MDLGEOSETANIMSECTION> geosetAnims;
  TSGrowableArray<MDLGENOBJECT *>       objects;
  TSGrowableArray<MDLBONESECTION>       bones;
  TSGrowableArray<MDLLIGHTSECTION>      lights;
  TSGrowableArray<MDLGENOBJECT>         helpers;
  TSGrowableArray<MDLATTACHMENTSECTION> attachments;
  TSGrowableArray<NTempest::C3Vector>   pivotPoints;
  TSGrowableArray<MDLPARTICLEEMITTER>   particleEmitters;
  TSGrowableArray<MDLCAMERASECTION>     cameras;
  TSGrowableArray<MDLEVENTSECTION>      events;
  TSGrowableArray<MDLPARTICLEEMITTER2>  particleEmitters2;
  TSGrowableArray<MDLHITTESTSHAPE>      hitTestShapes;
  TSGrowableArray<MDLRIBBONEMITTER>     ribbonEmitters;
  MDLCOLLISION                          collision;
};

struct MDLDATA : public MDLBASE {};
