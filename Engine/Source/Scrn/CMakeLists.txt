add_library(Scrn STATIC
    LAYER.CPP
    PERF.CPP
    SCRN.CPP
    STOCK.CPP
)

target_include_directories(Scrn PRIVATE
    ..
    ../../../Storm/H
)

if(MSVC_VERSION EQUAL 1200)
    target_compile_options(Scrn PRIVATE /Gr /Z7)
endif()
