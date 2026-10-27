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
    HEADER_FILES
        model/futurenet-agent-types.h
        model/observation-provider.h
        model/action-handler.h
    LIBRARIES_TO_LINK
        ${libcore}
        ${libnetwork}
)
