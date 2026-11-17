#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>
#else
#define GL_GLEXT_LEGACY
#define GL_ARB_multitexture 1
#include <GL/gl.h>
#endif
