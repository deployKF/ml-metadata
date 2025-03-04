#!/bin/bash
# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
#
# This script assumes the standard setup on tensorflow Jenkins windows machines.
# It is NOT guaranteed to work on any other machine. Use at your own risk!
#
# REQUIREMENTS:
# * All installed in standard locations:
#   - JDK8, and JAVA_HOME set.
#   - Microsoft Visual Studio 2017 Community Edition
#   - Msys2
#   - Anaconda3
# * Bazel windows executable copied as "bazel.exe" and included in PATH.

# Use a temporary directory with a short name.
export TMPDIR=${TMPDIR:-"C:/tmp"}
export TMPDIR=$(cygpath -m "$TMPDIR")
mkdir -p "$TMPDIR"

# Add timestamps before each command.
export PS4='+ $(date) + '

# Set bash path
export BAZEL_SH=${BAZEL_SH:-"C:/tools/msys64/usr/bin/bash"}

export PYTHON_BASE_PATH="${PYTHON_DIRECTORY:-Program Files/Anaconda3}"

# Set the path to find bazel.
export PATH="/c/tools/bazel/:$PATH"

# Set Python path for ./configure
export PYTHON_BIN_PATH="C:/${PYTHON_BASE_PATH}/python.exe"
export PYTHON_LIB_PATH="C:/${PYTHON_BASE_PATH}/lib/site-packages"
export PIP_COMMAND="C:/${PYTHON_BASE_PATH}/Scripts/pip.exe"

# Add python into PATH, it's needed because gen_git_source.py uses
# '/usr/bin/env python' as a shebang
export PATH="/c/${PYTHON_BASE_PATH}:$PATH"
# Add git into PATH needed for gen_git_source.py
export PATH="/c/Program Files/Git/cmd:$PATH"

# Make sure we have pip in PATH
export PATH="/c/${PYTHON_BASE_PATH}/Scripts:$PATH"
