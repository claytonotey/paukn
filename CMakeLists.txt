cmake_minimum_required (VERSION 3.4.3)

project(paukn)
set(paukn_sources
    src/params.cpp
    src/filter.cpp
    src/dwgs.cpp
    src/processor.cpp
    src/controller.cpp
    src/voice.cpp
    src/syncvoice.cpp
    src/factory.cpp
)

if(NOT DEFINED VST3_SDK_DIR)
  include(FetchContent)
  FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG        v3.7.4_build_25
    )
  
  FetchContent_Populate(vst3sdk)
  set(VST3_SDK_DIR ${vst3sdk_SOURCE_DIR} )
endif()
 
add_subdirectory(${VST3_SDK_DIR} ${CMAKE_CURRENT_BINARY_DIR}/vst3sdk)

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

target_compile_options(paukn PRIVATE -msse)
target_link_options(paukn PRIVATE -static)
