#include "GlExtSupport.h"

#include <gl/gl.h>

#include <storm.h>

#include <stdio.h>
#include <string.h>

int           glARBFragmentProgram;
int           glATIFragmentShader;
int           glExtCVA;
int           glExtBgra;
int           glExtClampToEdge;
int           glExtDrawRangeElements;
int           glExtMultiTextureCount;
int           glExtTextureCompression;
int           glExtTextureCompressionS3tc;
int           glExtTextureFilterAnisotropic;
int           glExtTextureLodBias;
int           glNVRegisterCombiners;
int           glNVRegisterCombiners2;
int           glNVTextureShader;
int           glNVTextureShader2;
int           glNVTextureShader3;
int           glNVVertexArrayRange;
int           glNVVertexArrayRange2;
int           glSGISGenerateMipmap;
int           glSGISTextureLod;
unsigned long glVersion;
int           wglARBPbuffer;
int           wglARBPixelFormat;
int           wglEXTSwapControl;
int(APIENTRY *wglSwapIntervalEXT)(int);
void(APIENTRY *glMultiTexCoord1dARB)(unsigned int, double);
void(APIENTRY *glMultiTexCoord1dvARB)(unsigned int, const double *);
void(APIENTRY *glMultiTexCoord1fARB)(unsigned int, float);
void(APIENTRY *glMultiTexCoord1fvARB)(unsigned int, const float *);
void(APIENTRY *glMultiTexCoord1iARB)(unsigned int, int);
void(APIENTRY *glMultiTexCoord1ivARB)(unsigned int, const int *);
void(APIENTRY *glMultiTexCoord1sARB)(unsigned int, short);
void(APIENTRY *glMultiTexCoord1svARB)(unsigned int, const short *);
void(APIENTRY *glMultiTexCoord2dARB)(unsigned int, double, double);
void(APIENTRY *glMultiTexCoord2dvARB)(unsigned int, const double *);
void(APIENTRY *glMultiTexCoord2fARB)(unsigned int, float, float);
void(APIENTRY *glMultiTexCoord2fvARB)(unsigned int, const float *);
void(APIENTRY *glMultiTexCoord2iARB)(unsigned int, int, int);
void(APIENTRY *glMultiTexCoord2ivARB)(unsigned int, const int *);
void(APIENTRY *glMultiTexCoord2sARB)(unsigned int, short, short);
void(APIENTRY *glMultiTexCoord2svARB)(unsigned int, const short *);
void(APIENTRY *glMultiTexCoord3dARB)(unsigned int, double, double, double);
void(APIENTRY *glMultiTexCoord3dvARB)(unsigned int, const double *);
void(APIENTRY *glMultiTexCoord3fARB)(unsigned int, float, float, float);
void(APIENTRY *glMultiTexCoord3fvARB)(unsigned int, const float *);
void(APIENTRY *glMultiTexCoord3iARB)(unsigned int, int, int, int);
void(APIENTRY *glMultiTexCoord3ivARB)(unsigned int, const int *);
void(APIENTRY *glMultiTexCoord3sARB)(unsigned int, short, short, short);
void(APIENTRY *glMultiTexCoord3svARB)(unsigned int, const short *);
void(APIENTRY *glMultiTexCoord4dARB)(unsigned int, double, double, double, double);
void(APIENTRY *glMultiTexCoord4dvARB)(unsigned int, const double *);
void(APIENTRY *glMultiTexCoord4fARB)(unsigned int, float, float, float, float);
void(APIENTRY *glMultiTexCoord4fvARB)(unsigned int, const float *);
void(APIENTRY *glMultiTexCoord4iARB)(unsigned int, int, int, int, int);
void(APIENTRY *glMultiTexCoord4ivARB)(unsigned int, const int *);
void(APIENTRY *glMultiTexCoord4sARB)(unsigned int, short, short, short, short);
void(APIENTRY *glMultiTexCoord4svARB)(unsigned int, const short *);
void(APIENTRY *glActiveTextureARB)(unsigned int);
void(APIENTRY *glClientActiveTextureARB)(unsigned int);
void(APIENTRY *glGenProgramsARB)(int, unsigned int *);
void(APIENTRY *glDeleteProgramsARB)(int, const unsigned int *);
void(APIENTRY *glBindProgramARB)(unsigned int, unsigned int);
void(APIENTRY *glProgramStringARB)(unsigned int, unsigned int, int, const void *);
void(APIENTRY *glProgramLocalParameter4fvARB)(unsigned int, unsigned int, const float *);
void(APIENTRY *glLockArraysEXT)(int, int);
void(APIENTRY *glUnlockArraysEXT)();
void(APIENTRY *glCompressedTexImage2DARB)(unsigned int, int, unsigned int, int, int, int, int, const void *);
void(APIENTRY *glCompressedTexSubImage2DARB)(unsigned int, int, int, int, int, int, unsigned int, int, const void *);
void(APIENTRY *glVertexArrayRangeNV)(int, const void *);
void(APIENTRY *glFlushVertexArrayRangeNV)();
void *(APIENTRY *wglAllocateMemoryNV)(int, float, float, float);
void(APIENTRY *wglFreeMemoryNV)(void *);
void(APIENTRY *glGenFencesNV)(int, unsigned int *);
void(APIENTRY *glDeleteFencesNV)(int, const unsigned int *);
void(APIENTRY *glFinishFenceNV)(unsigned int);
void(APIENTRY *glSetFenceNV)(unsigned int, unsigned int);
unsigned char(APIENTRY *glTestFenceNV)(unsigned int);
void(APIENTRY *glDrawRangeElementsEXT)(unsigned int, unsigned int, unsigned int, int, unsigned int, const void *);
void(APIENTRY *glCombinerInputNV)(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
void(APIENTRY *glCombinerOutputNV)(
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
void(APIENTRY *glFinalCombinerInputNV)(unsigned int, unsigned int, unsigned int, unsigned int);
void(APIENTRY *glCombinerParameterfvNV)(unsigned int, const float *);
void(APIENTRY *glCombinerParameteriNV)(unsigned int, int);
void(APIENTRY *glCombinerStageParameterfvNV)(unsigned int, unsigned int, const float *);
unsigned char(APIENTRY *glIsProgramARB)(unsigned int);
HPBUFFERARB__ *(APIENTRY *wglCreatePbufferARB)(HDC, int, int, int, const int *);
HDC(APIENTRY *wglGetPbufferDCARB)(HPBUFFERARB__ *);
int(APIENTRY *wglQueryPbufferARB)(HPBUFFERARB__ *, int, int *);
int(APIENTRY *wglReleasePbufferDCARB)(HPBUFFERARB__ *, HDC);
int(APIENTRY *wglDestroyPbufferARB)(HPBUFFERARB__ *);

WGLGETEXTENSIONSSTRINGARB    wglGetExtensionsStringARB;
WGLGETPIXELFORMATATTRIBIVARB wglGetPixelFormatAttribivARB;
WGLGETPIXELFORMATATTRIBFVARB wglGetPixelFormatAttribfvARB;
WGLCHOOSEPIXELFORMATARB      wglChoosePixelFormatARB;

static const char *s_glExts;
static const char *s_wglExts;

void __fastcall BindGlExtensions() {
  int         maxIdxs;
  int         versionLow = -1;
  int         versionHigh = -1;
  const char *compressedTexSubImageProcName = 0;
  int         maxVerts;
  const char *version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
  sscanf(version, "%d.%d", &versionHigh, &versionLow);
  glVersion = (versionHigh << 16) | versionLow;
  if (glVersion < 0x10002) {
    FATALASSERT(0);
  }

  glExtCVA = FindGlExt("GL_EXT_compiled_vertex_array");
  if (glExtCVA) {
    glLockArraysEXT = reinterpret_cast<void(APIENTRY *)(int, int)>(wglGetProcAddress("glLockArraysEXT"));
    glUnlockArraysEXT = reinterpret_cast<void(APIENTRY *)()>(wglGetProcAddress("glUnlockArraysEXT"));
  }

  if (FindGlExt("GL_ARB_multitexture")) {
    glGetIntegerv(0x84E2, &glExtMultiTextureCount);
    glMultiTexCoord1dARB = reinterpret_cast<void(APIENTRY *)(unsigned int, double)>(wglGetProcAddress("glMultiTexCoord1dARB"));
    glMultiTexCoord1dvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const double *)>(wglGetProcAddress("glMultiTexCoord1dvARB"));
    glMultiTexCoord1fARB = reinterpret_cast<void(APIENTRY *)(unsigned int, float)>(wglGetProcAddress("glMultiTexCoord1fARB"));
    glMultiTexCoord1fvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const float *)>(wglGetProcAddress("glMultiTexCoord1fvARB"));
    glMultiTexCoord1iARB = reinterpret_cast<void(APIENTRY *)(unsigned int, int)>(wglGetProcAddress("glMultiTexCoord1iARB"));
    glMultiTexCoord1ivARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const int *)>(wglGetProcAddress("glMultiTexCoord1ivARB"));
    glMultiTexCoord1sARB = reinterpret_cast<void(APIENTRY *)(unsigned int, short)>(wglGetProcAddress("glMultiTexCoord1sARB"));
    glMultiTexCoord1svARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const short *)>(wglGetProcAddress("glMultiTexCoord1svARB"));
    glMultiTexCoord2dARB = reinterpret_cast<void(APIENTRY *)(unsigned int, double, double)>(wglGetProcAddress("glMultiTexCoord2dARB"));
    glMultiTexCoord2dvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const double *)>(wglGetProcAddress("glMultiTexCoord2dvARB"));
    glMultiTexCoord2fARB = reinterpret_cast<void(APIENTRY *)(unsigned int, float, float)>(wglGetProcAddress("glMultiTexCoord2fARB"));
    glMultiTexCoord2fvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const float *)>(wglGetProcAddress("glMultiTexCoord2fvARB"));
    glMultiTexCoord2iARB = reinterpret_cast<void(APIENTRY *)(unsigned int, int, int)>(wglGetProcAddress("glMultiTexCoord2iARB"));
    glMultiTexCoord2ivARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const int *)>(wglGetProcAddress("glMultiTexCoord2ivARB"));
    glMultiTexCoord2sARB = reinterpret_cast<void(APIENTRY *)(unsigned int, short, short)>(wglGetProcAddress("glMultiTexCoord2sARB"));
    glMultiTexCoord2svARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const short *)>(wglGetProcAddress("glMultiTexCoord2svARB"));
    glMultiTexCoord3dARB = reinterpret_cast<void(APIENTRY *)(unsigned int, double, double, double)>(wglGetProcAddress("glMultiTexCoord3dARB"));
    glMultiTexCoord3dvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const double *)>(wglGetProcAddress("glMultiTexCoord3dvARB"));
    glMultiTexCoord3fARB = reinterpret_cast<void(APIENTRY *)(unsigned int, float, float, float)>(wglGetProcAddress("glMultiTexCoord3fARB"));
    glMultiTexCoord3fvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const float *)>(wglGetProcAddress("glMultiTexCoord3fvARB"));
    glMultiTexCoord3iARB = reinterpret_cast<void(APIENTRY *)(unsigned int, int, int, int)>(wglGetProcAddress("glMultiTexCoord3iARB"));
    glMultiTexCoord3ivARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const int *)>(wglGetProcAddress("glMultiTexCoord3ivARB"));
    glMultiTexCoord3sARB = reinterpret_cast<void(APIENTRY *)(unsigned int, short, short, short)>(wglGetProcAddress("glMultiTexCoord3sARB"));
    glMultiTexCoord3svARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const short *)>(wglGetProcAddress("glMultiTexCoord3svARB"));
    glMultiTexCoord4dARB =
        reinterpret_cast<void(APIENTRY *)(unsigned int, double, double, double, double)>(wglGetProcAddress("glMultiTexCoord4dARB"));
    glMultiTexCoord4dvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const double *)>(wglGetProcAddress("glMultiTexCoord4dvARB"));
    glMultiTexCoord4fARB = reinterpret_cast<void(APIENTRY *)(unsigned int, float, float, float, float)>(wglGetProcAddress("glMultiTexCoord4fARB"));
    glMultiTexCoord4fvARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const float *)>(wglGetProcAddress("glMultiTexCoord4fvARB"));
    glMultiTexCoord4iARB = reinterpret_cast<void(APIENTRY *)(unsigned int, int, int, int, int)>(wglGetProcAddress("glMultiTexCoord4iARB"));
    glMultiTexCoord4ivARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const int *)>(wglGetProcAddress("glMultiTexCoord4ivARB"));
    glMultiTexCoord4sARB = reinterpret_cast<void(APIENTRY *)(unsigned int, short, short, short, short)>(wglGetProcAddress("glMultiTexCoord4sARB"));
    glMultiTexCoord4svARB = reinterpret_cast<void(APIENTRY *)(unsigned int, const short *)>(wglGetProcAddress("glMultiTexCoord4svARB"));
    glActiveTextureARB = reinterpret_cast<void(APIENTRY *)(unsigned int)>(wglGetProcAddress("glActiveTextureARB"));
    glClientActiveTextureARB = reinterpret_cast<void(APIENTRY *)(unsigned int)>(wglGetProcAddress("glClientActiveTextureARB"));
  } else {
    glExtMultiTextureCount = 1;
  }

  glExtBgra = FindGlExt("GL_EXT_bgra");
  glExtClampToEdge = FindGlExt("GL_EXT_texture_edge_clamp");
  glExtTextureLodBias = FindGlExt("GL_EXT_texture_lod_bias");

  const char *compressedTexImageProcName = 0;
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
    glCompressedTexImage2DARB = reinterpret_cast<void(APIENTRY *)(unsigned int, int, unsigned int, int, int, int, int, const void *)>(
        wglGetProcAddress(compressedTexImageProcName)
    );
    glCompressedTexSubImage2DARB = reinterpret_cast<void(APIENTRY *)(unsigned int, int, int, int, int, int, unsigned int, int, const void *)>(
        wglGetProcAddress(compressedTexSubImageProcName)
    );
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
    glVertexArrayRangeNV = reinterpret_cast<void(APIENTRY *)(int, const void *)>(wglGetProcAddress("glVertexArrayRangeNV"));
    glFlushVertexArrayRangeNV = reinterpret_cast<void(APIENTRY *)()>(wglGetProcAddress("glFlushVertexArrayRangeNV"));
    wglAllocateMemoryNV = reinterpret_cast<void *(APIENTRY *)(int, float, float, float)>(wglGetProcAddress("wglAllocateMemoryNV"));
    wglFreeMemoryNV = reinterpret_cast<void(APIENTRY *)(void *)>(wglGetProcAddress("wglFreeMemoryNV"));
    glGenFencesNV = reinterpret_cast<void(APIENTRY *)(int, unsigned int *)>(wglGetProcAddress("glGenFencesNV"));
    glDeleteFencesNV = reinterpret_cast<void(APIENTRY *)(int, const unsigned int *)>(wglGetProcAddress("glDeleteFencesNV"));
    glFinishFenceNV = reinterpret_cast<void(APIENTRY *)(unsigned int)>(wglGetProcAddress("glFinishFenceNV"));
    glSetFenceNV = reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int)>(wglGetProcAddress("glSetFenceNV"));
    glTestFenceNV = reinterpret_cast<unsigned char(APIENTRY *)(unsigned int)>(wglGetProcAddress("glTestFenceNV"));
  }

  if (FindGlExt("GL_EXT_draw_range_elements")) {
    glExtDrawRangeElements = 1;
    glDrawRangeElementsEXT = reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int, unsigned int, int, unsigned int, const void *)>(
        wglGetProcAddress("glDrawRangeElementsEXT")
    );
    glGetIntegerv(0x80E8, &maxVerts);
    glGetIntegerv(0x80E9, &maxIdxs);
  }

  glSGISTextureLod = 0;
  if (FindGlExt("GL_NV_register_combiners")) {
    glNVRegisterCombiners = 1;
    glCombinerInputNV = reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)>(
        wglGetProcAddress("glCombinerInputNV")
    );
    glCombinerOutputNV = reinterpret_cast<void(APIENTRY *)(
        unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned char, unsigned char, unsigned char
    )>(wglGetProcAddress("glCombinerOutputNV"));
    glFinalCombinerInputNV =
        reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int, unsigned int, unsigned int)>(wglGetProcAddress("glFinalCombinerInputNV"));
    glCombinerParameterfvNV = reinterpret_cast<void(APIENTRY *)(unsigned int, const float *)>(wglGetProcAddress("glCombinerParameterfvNV"));
    glCombinerParameteriNV = reinterpret_cast<void(APIENTRY *)(unsigned int, int)>(wglGetProcAddress("glCombinerParameteriNV"));
  }
  if (FindGlExt("GL_NV_register_combiners2")) {
    glNVRegisterCombiners2 = 1;
    glCombinerStageParameterfvNV =
        reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int, const float *)>(wglGetProcAddress("glCombinerStageParameterfvNV"));
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
    glProgramStringARB = reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int, int, const void *)>(wglGetProcAddress("glProgramStringARB"));
    glBindProgramARB = reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int)>(wglGetProcAddress("glBindProgramARB"));
    glProgramLocalParameter4fvARB =
        reinterpret_cast<void(APIENTRY *)(unsigned int, unsigned int, const float *)>(wglGetProcAddress("glProgramLocalParameter4fvARB"));
    glDeleteProgramsARB = reinterpret_cast<void(APIENTRY *)(int, const unsigned int *)>(wglGetProcAddress("glDeleteProgramsARB"));
    glGenProgramsARB = reinterpret_cast<void(APIENTRY *)(int, unsigned int *)>(wglGetProcAddress("glGenProgramsARB"));
    glIsProgramARB = reinterpret_cast<unsigned char(APIENTRY *)(unsigned int)>(wglGetProcAddress("glIsProgramARB"));
  }

  wglGetExtensionsStringARB = reinterpret_cast<WGLGETEXTENSIONSSTRINGARB>(wglGetProcAddress("wglGetExtensionsStringARB"));
  if (FindWglExt("WGL_ARB_pixel_format")) {
    wglARBPixelFormat = 1;
    wglGetPixelFormatAttribivARB = reinterpret_cast<WGLGETPIXELFORMATATTRIBIVARB>(wglGetProcAddress("wglGetPixelFormatAttribivARB"));
    wglGetPixelFormatAttribfvARB = reinterpret_cast<WGLGETPIXELFORMATATTRIBFVARB>(wglGetProcAddress("wglGetPixelFormatAttribfvARB"));
    wglChoosePixelFormatARB = reinterpret_cast<WGLCHOOSEPIXELFORMATARB>(wglGetProcAddress("wglChoosePixelFormatARB"));
  }
  if (FindWglExt("WGL_ARB_pbuffer")) {
    wglARBPbuffer = 1;
    wglCreatePbufferARB = reinterpret_cast<HPBUFFERARB__ *(APIENTRY *)(HDC, int, int, int, const int *)>(wglGetProcAddress("wglCreatePbufferARB"));
    wglGetPbufferDCARB = reinterpret_cast<HDC(APIENTRY *)(HPBUFFERARB__ *)>(wglGetProcAddress("wglGetPbufferDCARB"));
    wglReleasePbufferDCARB = reinterpret_cast<int(APIENTRY *)(HPBUFFERARB__ *, HDC)>(wglGetProcAddress("wglReleasePbufferDCARB"));
    wglDestroyPbufferARB = reinterpret_cast<int(APIENTRY *)(HPBUFFERARB__ *)>(wglGetProcAddress("wglDestroyPbufferARB"));
    wglQueryPbufferARB = reinterpret_cast<int(APIENTRY *)(HPBUFFERARB__ *, int, int *)>(wglGetProcAddress("wglQueryPbufferARB"));
  }
  if (FindWglExt("WGL_EXT_swap_control")) {
    wglEXTSwapControl = 1;
    wglSwapIntervalEXT = reinterpret_cast<int(APIENTRY *)(int)>(wglGetProcAddress("wglSwapIntervalEXT"));
  }
}

void __fastcall UnbindGlExtensions() {
  s_glExts = 0;
  s_wglExts = 0;
}

static bool __fastcall ScanString(const char *string, const char *ext) {
  unsigned int length = strlen(ext);
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

bool __fastcall FindGlExt(const char *ext) {
  if (!s_glExts) {
    s_glExts = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
  }
  return ScanString(s_glExts, ext);
}

bool __fastcall FindWglExt(const char *ext) {
  if (!wglGetExtensionsStringARB) {
    return false;
  }
  if (!s_wglExts) {
    s_wglExts = wglGetExtensionsStringARB(wglGetCurrentDC());
  }
  return ScanString(s_wglExts, ext);
}
