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
struct MDLGLOBALSEQSECTION {
  unsigned int length;
};
struct MDLTEXANIMSECTION {
  MDLKEYTRACK<NTempest::C3Vector>     transkeys;
  MDLKEYTRACK<NTempest::C4Quaternion> rotkeys;
  MDLKEYTRACK<NTempest::C3Vector>     scalekeys;
};
struct MDLBONESECTION : public MDLGENOBJECT {
  unsigned int geosetId;
  unsigned int geosetAnimId;
};
enum LIGHT_TYPE {
  LIGHTTYPE_OMNI = 0,
  LIGHTTYPE_DIRECT = 1,
  LIGHTTYPE_AMBIENT = 2,
  NUM_MDL_LIGHT_TYPES = 3
};

struct MDLLIGHTSECTION : public MDLGENOBJECT {
  LIGHT_TYPE           type;
  MDLKEYTRACK<float>   attenstartkeys;
  float                staticAttenStart;
  MDLKEYTRACK<float>   attenendkeys;
  float                staticAttenEnd;
  MDLKEYTRACK<C3Color> colorkeys;
  C3Color              staticColor;
  MDLKEYTRACK<float>   intensitykeys;
  float                staticIntensity;
  MDLKEYTRACK<C3Color> ambcolorkeys;
  C3Color              staticAmbColor;
  MDLKEYTRACK<float>   ambintensitykeys;
  float                staticAmbIntensity;
  MDLKEYTRACK<float>   visibilityKeys;
};

struct MDLPARTICLE {
  CMdlString<260>    path;
  MDLKEYTRACK<float> life;
  float              staticLife;
  MDLKEYTRACK<float> speed;
  float              staticSpeed;
};

struct MDLPARTICLEEMITTER : public MDLGENOBJECT {
  MDLKEYTRACK<float> emissionRate;
  float              staticEmissionRate;
  MDLKEYTRACK<float> gravity;
  float              staticGravity;
  MDLKEYTRACK<float> longitude;
  float              staticLongitude;
  MDLKEYTRACK<float> latitude;
  float              staticLatitude;
  MDLPARTICLE        particle;
  MDLKEYTRACK<float> visibilityKeys;
};
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
struct MDLEVENTKEY {
  int time;
};

struct MDLEVENTSECTION : public MDLGENOBJECT {
  MDLSIMPLEKEYTRACK<MDLEVENTKEY> eventKeys;
};
struct MDLPARTICLEEMITTER2 : public MDLGENOBJECT {
  enum PARTICLE_EMITTER_TYPE {
    PET_BASE = 0,
    PET_PLANE = 1,
    PET_SPHERE = 2,
    PET_SPLINE = 3,
    NUM_PARTICLE_EMITTER_TYPES = 4
  };
  enum PARTICLE_BLEND_MODE {
    PBM_BLEND = 0,
    PBM_ADD = 1,
    PBM_MODULATE = 2,
    PBM_MODULATE_2X = 3,
    PBM_ALPHA_KEY = 4,
    NUM_PARTICLE_BLEND_MODES = 5
  };
  enum PARTICLE_TYPE {
    PT_HEAD = 0,
    PT_TAIL = 1,
    PT_BOTH = 2,
    NUM_PARTICLE_TYPES = 3
  };

  PARTICLE_EMITTER_TYPE            emitterType;
  float                            staticSpeed;
  MDLKEYTRACK<float>               speed;
  float                            staticVariation;
  MDLKEYTRACK<float>               variation;
  float                            staticLatitude;
  MDLKEYTRACK<float>               latitude;
  float                            staticLongitude;
  MDLKEYTRACK<float>               longitude;
  float                            staticGravity;
  MDLKEYTRACK<float>               gravity;
  float                            staticLife;
  MDLKEYTRACK<float>               life;
  float                            staticEmissionRate;
  MDLKEYTRACK<float>               emissionRate;
  float                            staticWidth;
  MDLKEYTRACK<float>               width;
  float                            staticLength;
  MDLKEYTRACK<float>               length;
  float                            staticZsource;
  MDLKEYTRACK<float>               zsource;
  PARTICLE_BLEND_MODE              blendMode;
  unsigned int                     rows;
  unsigned int                     cols;
  PARTICLE_TYPE                    type;
  float                            tailLength;
  float                            middleTime;
  C3Color                          startColor;
  C3Color                          middleColor;
  C3Color                          endColor;
  unsigned char                    startAlpha;
  unsigned char                    middleAlpha;
  unsigned char                    endAlpha;
  float                            startScale;
  float                            middleScale;
  float                            endScale;
  unsigned int                     lifespanUVAnimStart;
  unsigned int                     lifespanUVAnimEnd;
  unsigned int                     lifespanUVAnimRepeat;
  unsigned int                     decayUVAnimStart;
  unsigned int                     decayUVAnimEnd;
  unsigned int                     decayUVAnimRepeat;
  unsigned int                     tailUVAnimStart;
  unsigned int                     tailUVAnimEnd;
  unsigned int                     tailUVAnimRepeat;
  unsigned int                     tailDecayUVAnimStart;
  unsigned int                     tailDecayUVAnimEnd;
  unsigned int                     tailDecayUVAnimRepeat;
  MDLKEYTRACK<float>               visibilityKeys;
  unsigned int                     squirts;
  unsigned int                     textureId;
  int                              priorityPlane;
  unsigned int                     replaceableId;
  CMdlString<260>                  geometryMdl;
  CMdlString<260>                  recursionMdl;
  float                            twinkleFPS;
  float                            twinkleOnOff;
  float                            twinkleScaleMin;
  float                            twinkleScaleMax;
  float                            ivelScale;
  float                            tumblexMin;
  float                            tumblexMax;
  float                            tumbleyMin;
  float                            tumbleyMax;
  float                            tumblezMin;
  float                            tumblezMax;
  float                            drag;
  float                            spin;
  NTempest::C3Vector               windVector;
  float                            windTime;
  float                            followSpeed1;
  float                            followScale1;
  float                            followSpeed2;
  float                            followScale2;
  TSGrowableArray<NTempest::C3Vector> spline;
};
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
struct MDLRIBBONEMITTER : public MDLGENOBJECT {
  float                        staticHeightAbove;
  MDLKEYTRACK<float>           heightAbove;
  float                        staticHeightBelow;
  MDLKEYTRACK<float>           heightBelow;
  float                        staticAlpha;
  MDLKEYTRACK<float>           alphaKeys;
  C3Color                      staticColor;
  MDLKEYTRACK<C3Color>         colorKeys;
  unsigned int                 edgesPerSecond;
  float                        edgeLifetime;
  float                        gravity;
  unsigned int                 textureRows;
  unsigned int                 textureCols;
  unsigned int                 staticTextureSlot;
  MDLSIMPLEKEYTRACK<MDLINTKEY> textureSlot;
  MDLKEYTRACK<float>           visibilityKeys;
  unsigned int                 materialId;
};

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
