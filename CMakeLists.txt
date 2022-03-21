cmake_minimum_required (VERSION 3.4.3)

project(paukn)
set(paukn_sources
    src/params.cpp
    src/processor.cpp
    src/controller.cpp
    src/voice.cpp
    src/factory.cpp
    src/sinccoeffs.cpp
)

if(NOT DEFINED VST3_SDK_DIR)
  if(MINGW)
    set(VST3_SDK_PATCH_FILE ${CMAKE_CURRENT_LIST_DIR}/vst-validator-cmake.patch)
    set(VST3_SDK_PATCH_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR} && patch -Rsfp0 --dry-run < ${VST3_SDK_PATCH_FILE} || patch -p0 < ${VST3_SDK_PATCH_FILE})
  endif()
  message("Fetching vst3sdk...")
  include(FetchContent)
  FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG        v3.7.4_build_25
    PATCH_COMMAND ${VST3_SDK_PATCH_COMMAND}
    )
  
  FetchContent_Populate(vst3sdk)
  set(VST3_SDK_DIR ${vst3sdk_SOURCE_DIR} )
  set(VST3_SDK_BUILD_DIR ${vst3sdk_BUILD_DIR})
endif()
 
add_subdirectory(${VST3_SDK_DIR} ${VST3_SDK_BUILD_DIR})

set(target paukn)
smtg_add_vst3plugin(${target} ${paukn_sources})

target_link_libraries(${target}
    PRIVATE
     sdk
)
if(SMTG_MAC)
    smtg_target_set_bundle(${target}
    	BUNDLE_IDENTIFIER "mune.vst3.paukn"
    	INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/resource/Info.plist" PREPROCESS
    )
elseif(SMTG_WIN)
    target_sources(${target}
        PRIVATE
            resource/paukn.rc
    )
endif(SMTG_MAC)

target_compile_features(paukn PUBLIC cxx_std_17)
if(NOT MSVC)
  target_compile_options(paukn PRIVATE -msse)
endif()

if(MINGW)
  target_link_options(paukn PRIVATE -static)
endif()
