add_library(Tempest STATIC
    c22matrix.cpp
    c33matrix.cpp
    c34matrix.cpp
    c3spline.cpp
    c3vector.cpp
    c44matrix.cpp
    c4quaternion.cpp
    caabox.cpp
    cfacet.cpp
    cimvector.cpp
    cmath.cpp
    cmath_inline.cpp
    cmemblock.cpp
    crandom.cpp
    tempest_intersect.cpp
)

target_include_directories(Tempest PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Tempest PRIVATE /Gr /Z7)
endif()
