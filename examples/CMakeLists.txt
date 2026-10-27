# Copyright (c) 2026 FutureNet Team
# SPDX-License-Identifier: BSD-3-Clause
#
# Examples CMake configuration for M10: futurenet-agent

build_lib_example(
    NAME futurenet-agent-mock-control
    SOURCE_FILES futurenet-agent-mock-control.cc
    LIBRARIES_TO_LINK
        ${libfuturenet-agent}
        ${libcore}
        ${libnetwork}
)
