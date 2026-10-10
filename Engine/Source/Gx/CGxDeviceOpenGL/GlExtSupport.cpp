#include "GlExtSupport.h"

#include <gl/gl.h>

#include <storm.h>

#include <stdio.h>
#include <string.h>

BOOL  glARBFragmentProgram;
BOOL  glATIFragmentShader;
BOOL  glExtCVA;
BOOL  glExtBgra;
BOOL  glExtClampToEdge;
BOOL  glExtDrawRangeElements;
int   glExtMultiTextureCount;
BOOL  glExtTextureCompression;
BOOL  glExtTextureCompressionS3tc;
BOOL  glExtTextureFilterAnisotropic;
BOOL  glExtTextureLodBias;
BOOL  glNVRegisterCombiners;
BOOL  glNVRegisterCombiners2;
BOOL  glNVTextureShader;
BOOL  glNVTextureShader2;
BOOL  glNVTextureShader3;
BOOL  glNVVertexArrayRange;
BOOL  glNVVertexArrayRange2;
BOOL  glSGISGenerateMipmap;
BOOL  glSGISTextureLod;
DWORD glVersion;
BOOL  wglARBPbuffer;
BOOL  wglARBPixelFormat;
BOOL  wglEXTSwapControl;
int(APIENTRY *wglSwapIntervalEXT)(int);
void(APIENTRY *glMultiTexCoord1dARB)(UINT, double);
void(APIENTRY *glMultiTexCoord1dvARB)(UINT, const double *);
void(APIENTRY *glMultiTexCoord1fARB)(UINT, float);
void(APIENTRY *glMultiTexCoord1fvARB)(UINT, const float *);
void(APIENTRY *glMultiTexCoord1iARB)(UINT, int);
void(APIENTRY *glMultiTexCoord1ivARB)(UINT, const int *);
void(APIENTRY *glMultiTexCoord1sARB)(UINT, short);
void(APIENTRY *glMultiTexCoord1svARB)(UINT, const short *);
void(APIENTRY *glMultiTexCoord2dARB)(UINT, double, double);
void(APIENTRY *glMultiTexCoord2dvARB)(UINT, const double *);
void(APIENTRY *glMultiTexCoord2fARB)(UINT, float, float);
void(APIENTRY *glMultiTexCoord2fvARB)(UINT, const float *);
void(APIENTRY *glMultiTexCoord2iARB)(UINT, int, int);
void(APIENTRY *glMultiTexCoord2ivARB)(UINT, const int *);
void(APIENTRY *glMultiTexCoord2sARB)(UINT, short, short);
void(APIENTRY *glMultiTexCoord2svARB)(UINT, const short *);
void(APIENTRY *glMultiTexCoord3dARB)(UINT, double, double, double);
void(APIENTRY *glMultiTexCoord3dvARB)(UINT, const double *);
void(APIENTRY *glMultiTexCoord3fARB)(UINT, float, float, float);
void(APIENTRY *glMultiTexCoord3fvARB)(UINT, const float *);
void(APIENTRY *glMultiTexCoord3iARB)(UINT, int, int, int);
void(APIENTRY *glMultiTexCoord3ivARB)(UINT, const int *);
void(APIENTRY *glMultiTexCoord3sARB)(UINT, short, short, short);
void(APIENTRY *glMultiTexCoord3svARB)(UINT, const short *);
void(APIENTRY *glMultiTexCoord4dARB)(UINT, double, double, double, double);
void(APIENTRY *glMultiTexCoord4dvARB)(UINT, const double *);
void(APIENTRY *glMultiTexCoord4fARB)(UINT, float, float, float, float);
void(APIENTRY *glMultiTexCoord4fvARB)(UINT, const float *);
void(APIENTRY *glMultiTexCoord4iARB)(UINT, int, int, int, int);
void(APIENTRY *glMultiTexCoord4ivARB)(UINT, const int *);
void(APIENTRY *glMultiTexCoord4sARB)(UINT, short, short, short, short);
void(APIENTRY *glMultiTexCoord4svARB)(UINT, const short *);
void(APIENTRY *glActiveTextureARB)(UINT);
void(APIENTRY *glClientActiveTextureARB)(UINT);
void(APIENTRY *glGenProgramsARB)(int, UINT *);
void(APIENTRY *glDeleteProgramsARB)(int, const UINT *);
void(APIENTRY *glBindProgramARB)(UINT, UINT);
void(APIENTRY *glProgramStringARB)(UINT, UINT, int, LPCVOID);
void(APIENTRY *glProgramLocalParameter4fvARB)(UINT, UINT, const float *);
void(APIENTRY *glLockArraysEXT)(int, int);
void(APIENTRY *glUnlockArraysEXT)();
void(APIENTRY *glCompressedTexImage2DARB)(UINT, int, UINT, int, int, int, int, LPCVOID);
void(APIENTRY *glCompressedTexSubImage2DARB)(UINT, int, int, int, int, int, UINT, int, LPCVOID);
void(APIENTRY *glVertexArrayRangeNV)(int, LPCVOID);
void(APIENTRY *glFlushVertexArrayRangeNV)();
LPVOID(APIENTRY *wglAllocateMemoryNV)(int, float, float, float);
void(APIENTRY *wglFreeMemoryNV)(LPVOID);
void(APIENTRY *glGenFencesNV)(int, UINT *);
void(APIENTRY *glDeleteFencesNV)(int, const UINT *);
void(APIENTRY *glFinishFenceNV)(UINT);
void(APIENTRY *glSetFenceNV)(UINT, UINT);
BYTE(APIENTRY *glTestFenceNV)(UINT);
void(APIENTRY *glDrawRangeElementsEXT)(UINT, UINT, UINT, int, UINT, LPCVOID);
void(APIENTRY *glCombinerInputNV)(UINT, UINT, UINT, UINT, UINT, UINT);
void(APIENTRY *glCombinerOutputNV)(UINT, UINT, UINT, UINT, UINT, UINT, UINT, BYTE, BYTE, BYTE);
void(APIENTRY *glFinalCombinerInputNV)(UINT, UINT, UINT, UINT);
void(APIENTRY *glCombinerParameterfvNV)(UINT, const float *);
void(APIENTRY *glCombinerParameteriNV)(UINT, int);
void(APIENTRY *glCombinerStageParameterfvNV)(UINT, UINT, const float *);
BYTE(APIENTRY *glIsProgramARB)(UINT);
HPBUFFERARB(APIENTRY *wglCreatePbufferARB)(HDC, int, int, int, const int *);
HDC(APIENTRY *wglGetPbufferDCARB)(HPBUFFERARB);
int(APIENTRY *wglQueryPbufferARB)(HPBUFFERARB, int, int *);
int(APIENTRY *wglReleasePbufferDCARB)(HPBUFFERARB, HDC);
int(APIENTRY *wglDestroyPbufferARB)(HPBUFFERARB);

WGLGETEXTENSIONSSTRINGARB    wglGetExtensionsStringARB;
WGLGETPIXELFORMATATTRIBIVARB wglGetPixelFormatAttribivARB;
WGLGETPIXELFORMATATTRIBFVARB wglGetPixelFormatAttribfvARB;
WGLCHOOSEPIXELFORMATARB      wglChoosePixelFormatARB;

static LPCSTR s_glExts;
static LPCSTR s_wglExts;

void BindGlExtensions() {
  int    maxIdxs;
  int    versionLow = -1;
  int    versionHigh = -1;
  LPCSTR version = (LPCSTR)glGetString(GL_VERSION);
  sscanf(version, "%d.%d", &versionHigh, &versionLow);
  glVersion = (versionHigh << 16) | versionLow;
  if (glVersion < 0x10002) {
    FATALASSERT(0);
  }

  glExtCVA = FindGlExt("GL_EXT_compiled_vertex_array");
  if (glExtCVA) {
    glLockArraysEXT = (void(APIENTRY *)(int, int))wglGetProcAddress("glLockArraysEXT");
    glUnlockArraysEXT = (void(APIENTRY *)())wglGetProcAddress("glUnlockArraysEXT");
  }

  if (FindGlExt("GL_ARB_multitexture")) {
    glGetIntegerv(0x84E2, &glExtMultiTextureCount);
    glMultiTexCoord1dARB = (void(APIENTRY *)(UINT, double))wglGetProcAddress("glMultiTexCoord1dARB");
    glMultiTexCoord1dvARB = (void(APIENTRY *)(UINT, const double *))wglGetProcAddress("glMultiTexCoord1dvARB");
    glMultiTexCoord1fARB = (void(APIENTRY *)(UINT, float))wglGetProcAddress("glMultiTexCoord1fARB");
    glMultiTexCoord1fvARB = (void(APIENTRY *)(UINT, const float *))wglGetProcAddress("glMultiTexCoord1fvARB");
    glMultiTexCoord1iARB = (void(APIENTRY *)(UINT, int))wglGetProcAddress("glMultiTexCoord1iARB");
    glMultiTexCoord1ivARB = (void(APIENTRY *)(UINT, const int *))wglGetProcAddress("glMultiTexCoord1ivARB");
    glMultiTexCoord1sARB = (void(APIENTRY *)(UINT, short))wglGetProcAddress("glMultiTexCoord1sARB");
    glMultiTexCoord1svARB = (void(APIENTRY *)(UINT, const short *))wglGetProcAddress("glMultiTexCoord1svARB");
    glMultiTexCoord2dARB = (void(APIENTRY *)(UINT, double, double))wglGetProcAddress("glMultiTexCoord2dARB");
    glMultiTexCoord2dvARB = (void(APIENTRY *)(UINT, const double *))wglGetProcAddress("glMultiTexCoord2dvARB");
    glMultiTexCoord2fARB = (void(APIENTRY *)(UINT, float, float))wglGetProcAddress("glMultiTexCoord2fARB");
    glMultiTexCoord2fvARB = (void(APIENTRY *)(UINT, const float *))wglGetProcAddress("glMultiTexCoord2fvARB");
    glMultiTexCoord2iARB = (void(APIENTRY *)(UINT, int, int))wglGetProcAddress("glMultiTexCoord2iARB");
    glMultiTexCoord2ivARB = (void(APIENTRY *)(UINT, const int *))wglGetProcAddress("glMultiTexCoord2ivARB");
    glMultiTexCoord2sARB = (void(APIENTRY *)(UINT, short, short))wglGetProcAddress("glMultiTexCoord2sARB");
    glMultiTexCoord2svARB = (void(APIENTRY *)(UINT, const short *))wglGetProcAddress("glMultiTexCoord2svARB");
    glMultiTexCoord3dARB = (void(APIENTRY *)(UINT, double, double, double))wglGetProcAddress("glMultiTexCoord3dARB");
    glMultiTexCoord3dvARB = (void(APIENTRY *)(UINT, const double *))wglGetProcAddress("glMultiTexCoord3dvARB");
    glMultiTexCoord3fARB = (void(APIENTRY *)(UINT, float, float, float))wglGetProcAddress("glMultiTexCoord3fARB");
    glMultiTexCoord3fvARB = (void(APIENTRY *)(UINT, const float *))wglGetProcAddress("glMultiTexCoord3fvARB");
    glMultiTexCoord3iARB = (void(APIENTRY *)(UINT, int, int, int))wglGetProcAddress("glMultiTexCoord3iARB");
    glMultiTexCoord3ivARB = (void(APIENTRY *)(UINT, const int *))wglGetProcAddress("glMultiTexCoord3ivARB");
    glMultiTexCoord3sARB = (void(APIENTRY *)(UINT, short, short, short))wglGetProcAddress("glMultiTexCoord3sARB");
    glMultiTexCoord3svARB = (void(APIENTRY *)(UINT, const short *))wglGetProcAddress("glMultiTexCoord3svARB");
    glMultiTexCoord4dARB = (void(APIENTRY *)(UINT, double, double, double, double))wglGetProcAddress("glMultiTexCoord4dARB");
    glMultiTexCoord4dvARB = (void(APIENTRY *)(UINT, const double *))wglGetProcAddress("glMultiTexCoord4dvARB");
    glMultiTexCoord4fARB = (void(APIENTRY *)(UINT, float, float, float, float))wglGetProcAddress("glMultiTexCoord4fARB");
    glMultiTexCoord4fvARB = (void(APIENTRY *)(UINT, const float *))wglGetProcAddress("glMultiTexCoord4fvARB");
    glMultiTexCoord4iARB = (void(APIENTRY *)(UINT, int, int, int, int))wglGetProcAddress("glMultiTexCoord4iARB");
    glMultiTexCoord4ivARB = (void(APIENTRY *)(UINT, const int *))wglGetProcAddress("glMultiTexCoord4ivARB");
    glMultiTexCoord4sARB = (void(APIENTRY *)(UINT, short, short, short, short))wglGetProcAddress("glMultiTexCoord4sARB");
    glMultiTexCoord4svARB = (void(APIENTRY *)(UINT, const short *))wglGetProcAddress("glMultiTexCoord4svARB");
    glActiveTextureARB = (void(APIENTRY *)(UINT))wglGetProcAddress("glActiveTextureARB");
    glClientActiveTextureARB = (void(APIENTRY *)(UINT))wglGetProcAddress("glClientActiveTextureARB");
  } else {
    glExtMultiTextureCount = 1;
  }

  glExtBgra = FindGlExt("GL_EXT_bgra");
  glExtClampToEdge = FindGlExt("GL_EXT_texture_edge_clamp");
  glExtTextureLodBias = FindGlExt("GL_EXT_texture_lod_bias");

  LPCSTR compressedTexImageProcName = 0;
  LPCSTR compressedTexSubImageProcName = 0;
  if (FindGlExt("GL_ARB_texture_compression")) {
    compressedTexImageProcName = "glCompressedTexImage2DARB";
    compressedTexSubImageProcName = "glCompressedTexSubImage2DARB";
    glExtTextureCompression = 1;
  } else if (FindGlExt("GL_EXT_texture_compression")) {
    compressedTexImageProcName = "glCompressedTexImage2DEXT";
    compressedTexSubImageProcName = "glCompressedTexSubImage2DEXT";
    glExtTextureCompression = 1;
  }
  glExtTextureCompressionS3tc = FindGlExt("GL_EXT_texture_compression_s3tc");
  if (glExtTextureCompression && glExtTextureCompressionS3tc) {
    glCompressedTexImage2DARB =
        (void(APIENTRY *)(UINT, int, UINT, int, int, int, int, LPCVOID))wglGetProcAddress(compressedTexImageProcName);
    glCompressedTexSubImage2DARB =
        (void(APIENTRY *)(UINT, int, int, int, int, int, UINT, int, LPCVOID))wglGetProcAddress(compressedTexSubImageProcName);
  }
  if (!glCompressedTexImage2DARB || !glCompressedTexSubImage2DARB) {
    glExtTextureCompression = 0;
    glExtTextureCompressionS3tc = 0;
  }

  if (FindGlExt("GL_SGIS_generate_mipmap")) {
    glSGISGenerateMipmap = 1;
  }
  if (FindGlExt("GL_EXT_texture_filter_anisotropic")) {
    glExtTextureFilterAnisotropic = 1;
  }

  if (FindGlExt("GL_NV_vertex_array_range") && FindGlExt("GL_NV_vertex_array_range2") && FindGlExt("GL_NV_fence")) {
    glVertexArrayRangeNV = (void(APIENTRY *)(int, LPCVOID))wglGetProcAddress("glVertexArrayRangeNV");
    glFlushVertexArrayRangeNV = (void(APIENTRY *)())wglGetProcAddress("glFlushVertexArrayRangeNV");
    wglAllocateMemoryNV = (LPVOID(APIENTRY *)(int, float, float, float))wglGetProcAddress("wglAllocateMemoryNV");
    wglFreeMemoryNV = (void(APIENTRY *)(LPVOID))wglGetProcAddress("wglFreeMemoryNV");
    glGenFencesNV = (void(APIENTRY *)(int, UINT *))wglGetProcAddress("glGenFencesNV");
    glDeleteFencesNV = (void(APIENTRY *)(int, const UINT *))wglGetProcAddress("glDeleteFencesNV");
    glFinishFenceNV = (void(APIENTRY *)(UINT))wglGetProcAddress("glFinishFenceNV");
    glSetFenceNV = (void(APIENTRY *)(UINT, UINT))wglGetProcAddress("glSetFenceNV");
    glTestFenceNV = (BYTE(APIENTRY *)(UINT))wglGetProcAddress("glTestFenceNV");
  }

  if (FindGlExt("GL_EXT_draw_range_elements")) {
    int maxVerts;
    glExtDrawRangeElements = 1;
    glDrawRangeElementsEXT = (void(APIENTRY *)(UINT, UINT, UINT, int, UINT, LPCVOID))wglGetProcAddress("glDrawRangeElementsEXT");
    glGetIntegerv(0x80E8, &maxVerts);
    glGetIntegerv(0x80E9, &maxIdxs);
  }

  glSGISTextureLod = 0;
  if (FindGlExt("GL_NV_register_combiners")) {
    glNVRegisterCombiners = 1;
    glCombinerInputNV = (void(APIENTRY *)(UINT, UINT, UINT, UINT, UINT, UINT))wglGetProcAddress("glCombinerInputNV");
    glCombinerOutputNV =
        (void(APIENTRY *)(UINT, UINT, UINT, UINT, UINT, UINT, UINT, BYTE, BYTE, BYTE))wglGetProcAddress("glCombinerOutputNV");
    glFinalCombinerInputNV = (void(APIENTRY *)(UINT, UINT, UINT, UINT))wglGetProcAddress("glFinalCombinerInputNV");
    glCombinerParameterfvNV = (void(APIENTRY *)(UINT, const float *))wglGetProcAddress("glCombinerParameterfvNV");
    glCombinerParameteriNV = (void(APIENTRY *)(UINT, int))wglGetProcAddress("glCombinerParameteriNV");
  }
  if (FindGlExt("GL_NV_register_combiners2")) {
    glNVRegisterCombiners2 = 1;
    glCombinerStageParameterfvNV = (void(APIENTRY *)(UINT, UINT, const float *))wglGetProcAddress("glCombinerStageParameterfvNV");
  }
  if (FindGlExt("GL_NV_texture_shader")) {
    glNVTextureShader = 1;
  }
  if (FindGlExt("GL_NV_texture_shader2")) {
    glNVTextureShader2 = 1;
  }
  if (FindGlExt("GL_NV_texture_shader3")) {
    glNVTextureShader3 = 1;
  }
  if (FindGlExt("GL_ATI_fragment_shader")) {
    glATIFragmentShader = 1;
  }

  if (FindGlExt("GL_ARB_fragment_program")) {
    glARBFragmentProgram = 1;
    glProgramStringARB = (void(APIENTRY *)(UINT, UINT, int, LPCVOID))wglGetProcAddress("glProgramStringARB");
    glBindProgramARB = (void(APIENTRY *)(UINT, UINT))wglGetProcAddress("glBindProgramARB");
    glProgramLocalParameter4fvARB = (void(APIENTRY *)(UINT, UINT, const float *))wglGetProcAddress("glProgramLocalParameter4fvARB");
    glDeleteProgramsARB = (void(APIENTRY *)(int, const UINT *))wglGetProcAddress("glDeleteProgramsARB");
    glGenProgramsARB = (void(APIENTRY *)(int, UINT *))wglGetProcAddress("glGenProgramsARB");
    glIsProgramARB = (BYTE(APIENTRY *)(UINT))wglGetProcAddress("glIsProgramARB");
  }

  wglGetExtensionsStringARB = (WGLGETEXTENSIONSSTRINGARB)wglGetProcAddress("wglGetExtensionsStringARB");
  if (FindWglExt("WGL_ARB_pixel_format")) {
    wglARBPixelFormat = 1;
    wglGetPixelFormatAttribivARB = (WGLGETPIXELFORMATATTRIBIVARB)wglGetProcAddress("wglGetPixelFormatAttribivARB");
    wglGetPixelFormatAttribfvARB = (WGLGETPIXELFORMATATTRIBFVARB)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
    wglChoosePixelFormatARB = (WGLCHOOSEPIXELFORMATARB)wglGetProcAddress("wglChoosePixelFormatARB");
  }
  if (FindWglExt("WGL_ARB_pbuffer")) {
    wglARBPbuffer = 1;
    wglCreatePbufferARB = (HPBUFFERARB(APIENTRY *)(HDC, int, int, int, const int *))wglGetProcAddress("wglCreatePbufferARB");
    wglGetPbufferDCARB = (HDC(APIENTRY *)(HPBUFFERARB))wglGetProcAddress("wglGetPbufferDCARB");
    wglReleasePbufferDCARB = (int(APIENTRY *)(HPBUFFERARB, HDC))wglGetProcAddress("wglReleasePbufferDCARB");
    wglDestroyPbufferARB = (int(APIENTRY *)(HPBUFFERARB))wglGetProcAddress("wglDestroyPbufferARB");
    wglQueryPbufferARB = (int(APIENTRY *)(HPBUFFERARB, int, int *))wglGetProcAddress("wglQueryPbufferARB");
  }
  if (FindWglExt("WGL_EXT_swap_control")) {
    wglEXTSwapControl = 1;
    wglSwapIntervalEXT = (int(APIENTRY *)(int))wglGetProcAddress("wglSwapIntervalEXT");
  }
}

void UnbindGlExtensions() {
  s_glExts = 0;
  s_wglExts = 0;
}

static bool ScanString(LPCSTR string, LPCSTR ext) {
  UINT length = strlen(ext);
  while (*string) {
    if (!SStrCmpI(string, ext, length)) {
      return true;
    }
    while (*string && *string != ' ') {
      ++string;
    }
    while (*string == ' ') {
      ++string;
    }
  }
  return false;
}

bool FindGlExt(LPCSTR ext) {
  if (!s_glExts) {
    s_glExts = (LPCSTR)glGetString(GL_EXTENSIONS);
  }
  return ScanString(s_glExts, ext);
}

bool FindWglExt(LPCSTR ext) {
  if (!wglGetExtensionsStringARB) {
    return false;
  }
  if (!s_wglExts) {
    s_wglExts = wglGetExtensionsStringARB(wglGetCurrentDC());
  }
  return ScanString(s_wglExts, ext);
}
