#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(NEO_CORE_PREAMBLE_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/preamble_fixture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/preamble_tests.cpp
)

if(TESTS_XE_HPG_CORE)
  list(APPEND NEO_CORE_PREAMBLE_TESTS
       ${CMAKE_CURRENT_SOURCE_DIR}/test_preamble_xe_hpg_core.cpp
  )
endif()

target_sources(${TARGET_NAME} PRIVATE ${NEO_CORE_PREAMBLE_TESTS})
add_subdirectories()
