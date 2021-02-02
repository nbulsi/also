cmake_minimum_required(VERSION 3.8)
project(also LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX)
  # some specific compiler definitions
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-fcolor-diagnostics" HAS_FCOLOR_DIAGNOSTICS)
  if (HAS_FCOLOR_DIAGNOSTICS)
    add_compile_options(-fcolor-diagnostics)
  endif()

  # show quite some warnings (but remove some intentionally)
  add_compile_options(-W -Wall -Wextra)
  foreach (WARNING unknown-pragmas gnu-anonymous-struct nested-anon-types
      sign-compare unused-parameter format delete-non-virtual-dtor unused-lambda-capture
      unused-variable unused-private-field inconsistent-missing-override
      unused-but-set-parameter range-loop-analysis tautological-overlap-compare macro-redefined 
      cast-function-type)
    check_cxx_compiler_flag("-Wno-${WARNING}" HAS_WNO_${WARNING})
    if (HAS_WNO_${WARNING})
      add_compile_options(-Wno-${WARNING})
    endif()
  endforeach()
endif()

add_subdirectory(lib)
add_subdirectory(src)
