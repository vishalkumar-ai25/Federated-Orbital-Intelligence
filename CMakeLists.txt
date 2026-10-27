# Copyright (c) 2026 FutureNet Team
# SPDX-License-Identifier: BSD-3-Clause
#
# CMake configuration for M10: futurenet-agent

build_lib(
    LIBNAME futurenet-agent
    SOURCE_FILES
        model/futurenet-agent-types.cc
        model/observation-provider.cc
        model/action-handler.cc
        model/action-validator.cc
        model/action-executor.cc
    HEADER_FILES
        model/futurenet-agent-types.h
        model/observation-provider.h
        model/action-handler.h
        model/action-validator.h
        model/action-executor.h
    LIBRARIES_TO_LINK
        ${libcore}
        ${libnetwork}
    TEST_SOURCES
        test/futurenet-agent-test.cc
)
