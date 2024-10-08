/***************************************************************************/
/*                                                                         */
/*  psmodule.h                                                             */
/*                                                                         */
/*    High-level PSNames module interface (specification).                 */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef PSDRIVER_H
#define PSDRIVER_H

#include <freetype/ftmodule.h>


#ifdef __cplusplus
  extern "C" {
#endif


  FT_EXPORT_VAR( const FT_Module_Class )  psnames_module_class;

#ifdef __cplusplus
  }
#endif


#endif /* PSDRIVER_H */


/* END */
