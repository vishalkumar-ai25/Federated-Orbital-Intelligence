# Copyright (c) 2026 FutureNet Team
# SPDX-License-Identifier: BSD-3-Clause
#
# CMake configuration for M10: futurenet-agent

build_lib(
    LIBNAME futurenet-agent
    SOURCE_FILES
        model/futurenet-agent-types.cc
    HEADER_FILES
        model/futurenet-agent-types.h
    LIBRARIES_TO_LINK
        ${libcore}
        ${libnetwork}
)
