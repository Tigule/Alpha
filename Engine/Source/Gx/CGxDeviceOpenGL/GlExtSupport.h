#pragma once

#include <windows.h>

struct HPBUFFERARB__;

typedef const char *(APIENTRY *WGLGETEXTENSIONSSTRINGARB)(HDC);
typedef int(APIENTRY *WGLGETPIXELFORMATATTRIBIVARB)(HDC, int, int, unsigned int, const int *, int *);
typedef int(APIENTRY *WGLGETPIXELFORMATATTRIBFVARB)(HDC, int, int, unsigned int, const int *, float *);
typedef int(APIENTRY *WGLCHOOSEPIXELFORMATARB)(HDC, const int *, const float *, unsigned int, int *, unsigned int *);

enum {
  GL_BGRA_EXT = 0x80E1,
  GL_RGBA8_EXT = 0x8058,
  GL_RGBA4_EXT = 0x8056,
  GL_RGB5_A1_EXT = 0x8057,
  GL_RGB5_EXT = 0x8050,
  GL_CLAMP_TO_EDGE_EXT = 0x812F,
  GL_TEXTURE_MAX_LOD_SGIS = 0x813B,
  GL_GENERATE_MIPMAP_SGIS = 0x8191,
  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1,
  GL_COMPRESSED_RGBA_S3TC_DXT3_EXT = 0x83F2,
  GL_COMPRESSED_RGBA_S3TC_DXT5_EXT = 0x83F3,
  GL_UNSIGNED_SHORT_5_6_5_EXT = 0x8363,
  GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT = 0x8365,
  GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT = 0x8366,
  GL_UNSIGNED_INT_8_8_8_8_REV_EXT = 0x8367,
  GL_TEXTURE0_ARB = 0x84C0,
  GL_ALL_COMPLETED_NV = 0x84F2,
  GL_TEXTURE_FILTER_CONTROL_EXT = 0x8500,
  GL_TEXTURE_LOD_BIAS_EXT = 0x8501,
  GL_NORMAL_MAP_NV = 0x8511,
  GL_REFLECTION_MAP_NV = 0x8512,
  GL_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE,
  GL_VARIABLE_A_NV = 0x8523,
  GL_CONSTANT_COLOR0_NV = 0x852A,
  GL_CONSTANT_COLOR1_NV = 0x852B,
  GL_NUM_GENERAL_COMBINERS_NV = 0x854E,
  GL_COMBINER0_NV = 0x8550,
  GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF,
  GL_REGISTER_COMBINERS_NV = 0x8522,
  GL_VERTEX_ARRAY_RANGE_VALID_NV = 0x851F,
  GL_VERTEX_ARRAY_RANGE_NV = 0x8533,
  GL_PER_STAGE_CONSTANTS_NV = 0x8535,
  GL_TEXTURE_SHADER_NV = 0x86DE,
  GL_FRAGMENT_PROGRAM_ARB = 0x8804,
  GL_PROGRAM_FORMAT_ASCII_ARB = 0x8875,
  WGL_PBUFFER_LOST_ARB = 0x2036
};

extern int           glARBFragmentProgram;
extern int           glATIFragmentShader;
extern int           glExtCVA;
extern int           glExtBgra;
extern int           glExtClampToEdge;
extern int           glExtDrawRangeElements;
extern int           glExtMultiTextureCount;
extern int           glExtTextureCompression;
extern int           glExtTextureCompressionS3tc;
extern int           glExtTextureFilterAnisotropic;
extern int           glExtTextureLodBias;
extern int           glNVRegisterCombiners;
extern int           glNVRegisterCombiners2;
extern int           glNVTextureShader;
extern int           glNVTextureShader2;
extern int           glNVTextureShader3;
extern int           glNVVertexArrayRange;
extern int           glNVVertexArrayRange2;
extern int           glSGISGenerateMipmap;
extern int           glSGISTextureLod;
extern unsigned long glVersion;
extern int           wglARBPbuffer;
extern int           wglARBPixelFormat;
extern int           wglEXTSwapControl;
extern int(APIENTRY *wglSwapIntervalEXT)(int interval);
extern WGLGETEXTENSIONSSTRINGARB    wglGetExtensionsStringARB;
extern WGLGETPIXELFORMATATTRIBIVARB wglGetPixelFormatAttribivARB;
extern WGLGETPIXELFORMATATTRIBFVARB wglGetPixelFormatAttribfvARB;
extern WGLCHOOSEPIXELFORMATARB      wglChoosePixelFormatARB;
void BindGlExtensions();
void UnbindGlExtensions();
bool FindGlExt(const char *ext);
bool FindWglExt(const char *ext);
extern void(APIENTRY *glMultiTexCoord1dARB)(unsigned int, double);
extern void(APIENTRY *glMultiTexCoord1dvARB)(unsigned int, const double *);
extern void(APIENTRY *glMultiTexCoord1fARB)(unsigned int, float);
extern void(APIENTRY *glMultiTexCoord1fvARB)(unsigned int, const float *);
extern void(APIENTRY *glMultiTexCoord1iARB)(unsigned int, int);
extern void(APIENTRY *glMultiTexCoord1ivARB)(unsigned int, const int *);
extern void(APIENTRY *glMultiTexCoord1sARB)(unsigned int, short);
extern void(APIENTRY *glMultiTexCoord1svARB)(unsigned int, const short *);
extern void(APIENTRY *glMultiTexCoord2dARB)(unsigned int, double, double);
extern void(APIENTRY *glMultiTexCoord2dvARB)(unsigned int, const double *);
extern void(APIENTRY *glMultiTexCoord2fARB)(unsigned int, float, float);
extern void(APIENTRY *glMultiTexCoord2fvARB)(unsigned int, const float *);
extern void(APIENTRY *glMultiTexCoord2iARB)(unsigned int, int, int);
extern void(APIENTRY *glMultiTexCoord2ivARB)(unsigned int, const int *);
extern void(APIENTRY *glMultiTexCoord2sARB)(unsigned int, short, short);
extern void(APIENTRY *glMultiTexCoord2svARB)(unsigned int, const short *);
extern void(APIENTRY *glMultiTexCoord3dARB)(unsigned int, double, double, double);
extern void(APIENTRY *glMultiTexCoord3dvARB)(unsigned int, const double *);
extern void(APIENTRY *glMultiTexCoord3fARB)(unsigned int, float, float, float);
extern void(APIENTRY *glMultiTexCoord3fvARB)(unsigned int, const float *);
extern void(APIENTRY *glMultiTexCoord3iARB)(unsigned int, int, int, int);
extern void(APIENTRY *glMultiTexCoord3ivARB)(unsigned int, const int *);
extern void(APIENTRY *glMultiTexCoord3sARB)(unsigned int, short, short, short);
extern void(APIENTRY *glMultiTexCoord3svARB)(unsigned int, const short *);
extern void(APIENTRY *glMultiTexCoord4dARB)(unsigned int, double, double, double, double);
extern void(APIENTRY *glMultiTexCoord4dvARB)(unsigned int, const double *);
extern void(APIENTRY *glMultiTexCoord4fARB)(unsigned int, float, float, float, float);
extern void(APIENTRY *glMultiTexCoord4fvARB)(unsigned int, const float *);
extern void(APIENTRY *glMultiTexCoord4iARB)(unsigned int, int, int, int, int);
extern void(APIENTRY *glMultiTexCoord4ivARB)(unsigned int, const int *);
extern void(APIENTRY *glMultiTexCoord4sARB)(unsigned int, short, short, short, short);
extern void(APIENTRY *glMultiTexCoord4svARB)(unsigned int, const short *);
extern void(APIENTRY *glActiveTextureARB)(unsigned int texture);
extern void(APIENTRY *glClientActiveTextureARB)(unsigned int texture);
extern void(APIENTRY *glGenProgramsARB)(int count, unsigned int *programs);
extern void(APIENTRY *glDeleteProgramsARB)(int count, const unsigned int *programs);
extern void(APIENTRY *glBindProgramARB)(unsigned int target, unsigned int program);
extern void(APIENTRY *glProgramStringARB)(unsigned int target, unsigned int format, int length, const void *string);
extern void(APIENTRY *glProgramLocalParameter4fvARB)(unsigned int target, unsigned int index, const float *values);
extern void(APIENTRY *glLockArraysEXT)(int first, int count);
extern void(APIENTRY *glUnlockArraysEXT)();
extern void(APIENTRY *glCompressedTexImage2DARB)(unsigned int, int, unsigned int, int, int, int, int, const void *);
extern void(APIENTRY *glCompressedTexSubImage2DARB)(unsigned int, int, int, int, int, int, unsigned int, int, const void *);
extern void(APIENTRY *glVertexArrayRangeNV)(int, const void *);
extern void(APIENTRY *glFlushVertexArrayRangeNV)();
extern void *(APIENTRY *wglAllocateMemoryNV)(int, float, float, float);
extern void(APIENTRY *wglFreeMemoryNV)(void *);
extern void(APIENTRY *glGenFencesNV)(int, unsigned int *);
extern void(APIENTRY *glDeleteFencesNV)(int, const unsigned int *);
extern void(APIENTRY *glFinishFenceNV)(unsigned int);
extern void(APIENTRY *glSetFenceNV)(unsigned int, unsigned int);
extern unsigned char(APIENTRY *glTestFenceNV)(unsigned int);
extern void(APIENTRY *glDrawRangeElementsEXT)(unsigned int, unsigned int, unsigned int, int, unsigned int, const void *);
extern void(APIENTRY *glCombinerInputNV)(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
extern void(APIENTRY *glCombinerOutputNV)(
    unsigned int,
    unsigned int,
    unsigned int,
    unsigned int,
    unsigned int,
    unsigned int,
    unsigned int,
    unsigned char,
    unsigned char,
    unsigned char
);
extern void(APIENTRY *glFinalCombinerInputNV)(unsigned int, unsigned int, unsigned int, unsigned int);
extern void(APIENTRY *glCombinerParameterfvNV)(unsigned int, const float *);
extern void(APIENTRY *glCombinerParameteriNV)(unsigned int, int);
extern void(APIENTRY *glCombinerStageParameterfvNV)(unsigned int, unsigned int, const float *);
extern unsigned char(APIENTRY *glIsProgramARB)(unsigned int);
extern HPBUFFERARB__ *(APIENTRY *wglCreatePbufferARB)(HDC dc, int pixelFormat, int width, int height, const int *attributes);
extern HDC(APIENTRY *wglGetPbufferDCARB)(HPBUFFERARB__ *pbuffer);
extern int(APIENTRY *wglQueryPbufferARB)(HPBUFFERARB__ *pbuffer, int attribute, int *value);
extern int(APIENTRY *wglReleasePbufferDCARB)(HPBUFFERARB__ *pbuffer, HDC dc);
extern int(APIENTRY *wglDestroyPbufferARB)(HPBUFFERARB__ *pbuffer);
