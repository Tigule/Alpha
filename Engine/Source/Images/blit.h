#ifndef ENGINE_SOURCE_IMAGES_BLIT_H
#define ENGINE_SOURCE_IMAGES_BLIT_H

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

void __fastcall Blit(
    const NTempest::C2iVector &size,
    BlitAlpha                  alpha,
    const void                *src,
    unsigned int               srcStride,
    BlitFormat                 srcFmt,
    void                      *dst,
    unsigned int               dstStride,
    BlitFormat                 dstFmt
);
unsigned int __fastcall CalcRowStride(BlitFormat format, unsigned int width);

#endif
