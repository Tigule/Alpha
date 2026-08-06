#ifndef ENGINE_SOURCE_IMAGES_BLIT_H
#define ENGINE_SOURCE_IMAGES_BLIT_H

#include <Base/Base.h>

namespace NTempest {
  class C2iVector;
}

enum BlitFormat {
  BlitFormat_Unknown = 0,
  BlitFormat_Argb8888 = 1,
  BlitFormat_Argb4444 = 2,
  BlitFormat_Argb1555 = 3,
  BlitFormat_Rgb565 = 4,
  BlitFormat_Dxt1 = 5,
  BlitFormat_Dxt3 = 6,
  BlitFormat_Dxt5 = 7,
  BlitFormats_Last = 8
};

enum BlitAlpha {
  BlitAlpha_0 = 0,
  BlitAlpha_1 = 1,
  BlitAlpha_8 = 2,
  BlitAlpha_Filler = 3,
  BlitAlphas_Last = 4
};

void Blit(
    const NTempest::C2iVector &size,
    BlitAlpha                  alpha,
    LPCVOID                    src,
    UINT                       srcStride,
    BlitFormat                 srcFmt,
    LPVOID                     dst,
    UINT                       dstStride,
    BlitFormat                 dstFmt
);
UINT CalcRowStride(BlitFormat format, UINT width);

#endif
