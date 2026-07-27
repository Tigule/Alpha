#include "CGxDeviceOpenGl.h"
#include "GlExtSupport.h"

#include <Tempest/c4vector.h>

#include <gl/gl.h>

namespace RegisterCombiners {

  struct CombinerVariable {
    unsigned int input;
    unsigned int mapping;
    unsigned int component;
  };

  struct CombinerOutput {
    unsigned int  abOutput;
    unsigned int  cdOutput;
    unsigned int  sumOutput;
    unsigned int  scale;
    unsigned int  bias;
    unsigned char abDotProduct;
    unsigned char cdDotProduct;
    unsigned char muxSum;
  };

  struct CombinerPortion {
    enum {
      VarA = 0,
      VarB = 1,
      VarC = 2,
      VarD = 3,
      Var_Count = 4
    };

    CombinerVariable variable[4];
    CombinerOutput   output;

    void Realize(unsigned int stage, unsigned int portion);
  };

  struct GeneralCombiner {
    enum {
      Portion_Rgb = 0,
      Portion_Alpha = 1,
      Portion_Count = 2
    };

    CombinerPortion    portion[2];
    NTempest::C4Vector constants[2];

    void Realize(unsigned int stage, int perStageConstants);
  };

  struct FinalCombiner {
    enum {
      VarA = 0,
      VarB = 1,
      VarC = 2,
      VarD = 3,
      VarE = 4,
      VarF = 5,
      VarG = 6,
      Var_Count = 7
    };

    CombinerVariable variable[7];

    void Realize();
  };

  void CombinerPortion::Realize(unsigned int stage, unsigned int portion) {
    for (unsigned int i = 0; i < 4; ++i) {
      glCombinerInputNV(stage, portion, GL_VARIABLE_A_NV + i, variable[i].input, variable[i].mapping, variable[i].component);
    }
    glCombinerOutputNV(
        stage, portion, output.abOutput, output.cdOutput, output.sumOutput, output.scale, output.bias, output.abDotProduct, output.cdDotProduct,
        output.muxSum
    );
  }

  void GeneralCombiner::Realize(unsigned int stage, int perStageConstants) {
    portion[0].Realize(stage, GL_RGB);
    portion[1].Realize(stage, GL_ALPHA);
    if (perStageConstants) {
      glCombinerStageParameterfvNV(stage, GL_CONSTANT_COLOR0_NV, &constants[0].x);
      glCombinerStageParameterfvNV(stage, GL_CONSTANT_COLOR1_NV, &constants[1].x);
    }
  }

  void FinalCombiner::Realize() {
    for (unsigned int i = 0; i < 7; ++i) {
      glFinalCombinerInputNV(GL_VARIABLE_A_NV + i, variable[i].input, variable[i].mapping, variable[i].component);
    }
  }

}  // namespace RegisterCombiners

void CGxDeviceOpenGl::IPixelShaderBind(CGxPixelShader *ps) {
  if (ps) {
    if (m_caps.m_pixelShaderTarget == CGxPixelShader::Target_nvrc) {
      const unsigned char *code = ps->code.Ptr();
      unsigned int         combinerCount = *reinterpret_cast<const unsigned int *>(code);
      glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, combinerCount);
      glCombinerParameteriNV(GL_PER_STAGE_CONSTANTS_NV, code[4]);
      glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, reinterpret_cast<const float *>(code + 8));
      glCombinerParameterfvNV(GL_CONSTANT_COLOR1_NV, reinterpret_cast<const float *>(code + 24));

      RegisterCombiners::GeneralCombiner *general = reinterpret_cast<RegisterCombiners::GeneralCombiner *>(const_cast<unsigned char *>(code + 40));
      for (unsigned int stage = 0; stage < combinerCount; ++stage) {
        general[stage].Realize(GL_COMBINER0_NV + stage, 0);
      }
      reinterpret_cast<RegisterCombiners::FinalCombiner *>(const_cast<unsigned char *>(code + 392))->Realize();
      DsSet(Ds_RegisterCombinersNV, 1, 0);
    } else if (m_caps.m_pixelShaderTarget == CGxPixelShader::Target_arbfp1) {
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ps->apiSpecific);
      DsSet(Ds_FragmentProgramARB, 1, 0);
    }
  } else if (m_caps.m_pixelShaderTarget == CGxPixelShader::Target_nvrc) {
    DsSet(Ds_RegisterCombinersNV, 0, 0);
  } else if (m_caps.m_pixelShaderTarget == CGxPixelShader::Target_arbfp1) {
    DsSet(Ds_FragmentProgramARB, 0, 0);
  }
}

void CGxDeviceOpenGl::PixelShaderCreate(CGxPixelShader *&ps, const char *filename) {
  CGxDevice::PixelShaderCreate(ps, filename);

  if (ps->code.Count() && glARBFragmentProgram) {
    glGenProgramsARB(1, &ps->apiSpecific);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ps->apiSpecific);
    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, ps->code.Count(), ps->code.Ptr());
  }
}

void CGxDeviceOpenGl::PixelShaderDestroy(CGxPixelShader *&ps) {
  if (ps->refCount == 1 && ps->apiSpecific) {
    glDeleteProgramsARB(1, &ps->apiSpecific);
    ps->apiSpecific = 0;
  }

  CGxDevice::PixelShaderDestroy(ps);
}

void CGxDeviceOpenGl::ISetShaderParamList(TSExplicitList<CGxShaderParam, 108> &params, int forceForBind) {
  for (CGxShaderParam *param = params.Head(); param; param = params.Next(param)) {
    if (param->dirty || forceForBind) {
      for (unsigned int i = 0; i < CGxShaderParam::TypeCountTable[param->type]; ++i) {
        glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, param->index + i, param->f + i * 4);
      }

      param->dirty = 0;
    }
  }
}

void CGxDeviceOpenGl::IShaderForceRecreation() {
  CGxPixelShader *ps = m_pixelShaderList.Head();
  while (ps) {
    if (ps->apiSpecific) {
      glDeleteProgramsARB(1, &ps->apiSpecific);
      ps->apiSpecific = 0;
    }
    ps = m_pixelShaderList.Next(ps);
  }

  CGxVertexShader *vs = m_vertexShaderList.Head();
  while (vs) {
    vs = m_vertexShaderList.Next(vs);
  }
}
