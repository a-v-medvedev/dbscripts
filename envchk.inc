# vi:syntax=sh
#
# BSD 3-Clause License
#
# Copyright (c) 2019-2021, Boris Krasnopolsky, Alexey Medvedev
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

function verify_compiler_version {
    extension=$1
    compiler="$2"
    GNU_min_version="$3"
    INTEL_min_version="$4"
    CLANG_min_version="$5"
    cctype=$(verify_comp_type "$compiler" "$extension") && true
    [ "$?" == 0 ] || fatal "Cannot compile with $compiler compiler -- compiler is not able to produce result executable"
    ccvers=$(verify_comp_version "$cctype" "$compiler") && true
    [ $? == 0 ] || warning "Compiler $compiler doesn't provide version information it the expected form. Compiler type detected: $cctype"
    ccminvers=$(get_minimal_version_for_type "$cctype") && true
    [ $? == 0 ] || warning "Compiler $compiler not listed, can't get minimal version. Compiler type detected: $cctype"
    version_is_less "$ccvers" "$ccminvers"
    local ret=$?
    [ $ret == 0 ] && fatal "Compiler $compiler of version $ccvers is not supported; minimal supported version is $ccminvers"
    if [ "$ret" != 0 ]; then
        [ -z "$cctype" ] && cctype='<unknown>'
        [ -z "$ccvers" ] && ccvers='<unknown>'
        info "Using compiler $compiler, compiler type: $cctype, version $ccvers";
    fi
    return 0
}

function verify_cuda_version {
    local comp="$1"
    local cudaminvers="$2"
    local vout=$("$1" --version)
    echo "$vout" | grep -q 'nvcc: NVIDIA (R) Cuda compiler driver' || fatal "can't detect CUDA version"
    echo "$vout" | grep -q 'Cuda compilation tools, release' || fatal "can't detect CUDA version"
    export cudavers=$(echo "$vout" | awk 'BEGIN{RS=","} /release/ { print $2 }')
    version_is_less "$cudavers" "$cudaminvers" && fatal "CUDA version from path $CUDA_HOME is $cudavers; it is not supported; minimal supported version is ${cudaminvers}. NOTE: set DNB_NOCUDA environment variable to omit all CUDA checks.";
    info "Using CUDA path ${CUDA_HOME}, version: ${cudavers}" 
    return 0
}

function verify_cmake_version() {
    local cmakeminvers="$1"
    [ -z "$(which cmake)" ] && fatal "no cmake in PATH"
    local cmakevers=$(cmake --version | head -n 1 | awk '{print $3}')
	version_is_less "$cmakevers" "$cmakeminvers" && fatal "cmake of version $CMAKE_VERSION is not supported; minimal supported version is $CMAKE_MINIMAL_VERSION. NOTE: set DNB_NOCMAKE environment variable to omit cmake check."
	info "Using cmake version $cmakevers";
	return 0
}

function working_dir_check {
    set +u
    [ ! -z "$INSTALL_DIR" ] && return 0
    set -u
    [ -e "$0" ] && cs1=$(get_csum "$0")
    [ -e "$PWD/dnb.sh" ] && cs2=$(get_csum "$PWD/dnb.sh")
    if [ "$cs1" != "$cs2" ]; then
        info "NOTE: this script must be run from the directory it is in."
        info "NOTE: set INSTALL_DIR evironment variable if custom build and install directory required."
        fatal "can't continue from this current directory"
    fi
}

function verify_if_file_presents_in_cuda_path() {
    local file="$1"
    [ -f "$CUDA_HOME"/"$file" ] || fatal "can't find $file in CUDA path"
}

function cuda_home_dir_set {
    set +u
    if [ -z "$CUDA_HOME" ]; then
        local nvccbinary=$(which nvcc 2>/dev/null)
        [ -z "$nvccbinary" ] && fatal "can't find nvcc. Is CUDA in PATH? NOTE: set DNB_NOCUDA environment variable to omit all CUDA checks."
        export CUDA_HOME=$(get_uplevel_dir "$nvccbinary")
    fi
    set -u
    verify_if_file_presents_in_cuda_path bin/nvcc
    #verify_if_file_presents_in_cuda_path lib64/libnvToolsExt.so
    #verify_if_file_presents_in_cuda_path lib64/libcudart.so
    #verify_if_file_presents_in_cuda_path lib/x86_64-linux-gnu/libnvToolsExt.so
    #verify_if_file_presents_in_cuda_path lib/x86_64-linux-gnu/libcudart.so
}

function set_variable_if_not_defined() {
    local varname="$1"
    local value="$2"
    set +u
    local x=$(eval echo \$$varname)
    set -u
    [ -z "$x" ] && eval export $varname=$value
}

function set_defaults {
    set_variable_if_not_defined CC gcc
    set_variable_if_not_defined CXX g++
    set_variable_if_not_defined MPICXX mpicxx
    set_variable_if_not_defined MPICC mpicc
    set_variable_if_not_defined INSTALL_DIR $PWD

    set_variable_if_not_defined CMAKE_MINIMAL_VERSION "3.1.0"
    set_variable_if_not_defined GCC_MINIMAL_VERSION "5.0.0"
    set_variable_if_not_defined ICC_MINIMAL_VERSION "13.0.0"
# to be corrected
    set_variable_if_not_defined CLANG_MINIMAL_VERSION "7.0.0"
    set_variable_if_not_defined CUDA_MINIMAL_VERSION "7.0"

    set_variable_if_not_defined MAKE_PARALLEL_LEVEL 8
}

function environment_check {
    working_dir_check
    set_defaults 
    set +u
    if [ -z "$DNB_NOCMAKE" ]; then
        verify_cmake_version "$CMAKE_MINIMAL_VERSION" || return 1
    fi
    if [ -z "$DNB_NOCCOMP" ]; then
        verify_compiler_version c "$CC" "$GCC_MINIMAL_VERSION" "$ICC_MINIMAL_VERSION" "$CLANG_MINIMAL_VERSION" || return 1
    fi
    if [ -z "$DNB_NOCXXCOMP" ]; then
        verify_compiler_version cpp "$CXX" "$GCC_MINIMAL_VERSION" "$ICC_MINIMAL_VERSION" "$CLANG_MINIMAL_VERSION" || return 1
    fi
    if [ -z "$DNB_NOCCOMP" ]; then
        verify_compiler_version c "$MPICC" "$GCC_MINIMAL_VERSION" "$ICC_MINIMAL_VERSION" "$CLANG_MINIMAL_VERSION" || return 1
    fi
    if [ -z "$DNB_NOCXXCOMP" ]; then
        verify_compiler_version cpp "$MPICXX" "$GCC_MINIMAL_VERSION" "$ICC_MINIMAL_VERSION" "$CLANG_MINIMAL_VERSION" || return 1
    fi
    [ ! -d "$INSTALL_DIR" ] && fatal "target directory $INSTALL_DIR doesn't exist."
    set -u
    return 0
}

function environment_check_cuda {
    set_defaults
    cuda_home_dir_set
    verify_cuda_version "$CUDA_HOME/bin/nvcc" "$CUDA_MINIMAL_VERSION" || return 1
}

function environment_check_main {
    set +u
    [ -z "$env_init_global_is_declared" ] && check_if_bash_func_declared "env_init_global"
    [ -z "$env_init_is_declared" ] && check_if_bash_func_declared "env_init"
    if [ "$env_init_global_is_declared" == "1" ]; then
        env_init_global
    fi
    set -u
    environment_check
    set +u
    [ -z "$DNB_NOCUDA" ] && environment_check_cuda
    set -u
    return $?
}

function environment_check_specific {
    local name="$1"
    if [ "$env_init_is_declared" == "1" ]; then
        env_init "$name"
        if [ "1" == "$?" ]; then
            environment_check
            return $?
        fi
    fi
    return 0
}
