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

function dnb_psubmit() {
    generic_prolog "psubmit" $* || return 0	
    du_github "a-v-medvedev" "v"
    local FILES=""
    if this_mode_is_set "i"; then
        cd ${pkg}-${V}.src
        FILES=$(ls -1 *.sh)
        cd $DNB_INSTALL_DIR
    fi
    i_direct_copy "$FILES" "remove-before"
    generic_epilog
    return 0
}

function dnb_yaml-cpp() {
	generic_prolog "yaml-cpp" $* || return 0
    du_github "jbeder" "yaml-cpp-"
    if this_mode_is_set 'u'; then
        sed -i '/target_compile_options(yaml-cpp/,+15d' ${pkg}.src/CMakeLists.txt
    fi
    local OPTS=""
    OPTS="$OPTS -DYAML_BUILD_SHARED_LIBS=OFF"
    OPTS="$OPTS -DYAML_CPP_BUILD_TESTS=OFF"
    OPTS="$OPTS -DYAML_CPP_BUILD_TOOLS=OFF"
    OPTS="$OPTS -DYAML_CPP_BUILD_CONTRIB=OFF"
    bi_cmake ".." "$OPTS"
    if this_mode_is_set "i"; then
        if [ -d "$pkg"-"$V"/lib64 ]; then
            cd "$pkg"-"$V"
            [ -e lib ] || ln -sf lib64 lib
            cd $DNB_INSTALL_DIR
        fi
    fi
    generic_epilog
    return 0
}

function dnb_argsparser() {
    generic_prolog "argsparser" $* || return 0
	#local pkg="argsparser"
	#environment_check_specific "$pkg" || fatal "$pkg: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    set +u
    local argsparser_params="$ARGSPARSER_PARAMS"
    set -u
    #[ "$(check_this_pkg_record $pkg $V $m)" == "cached" ] && return 0
    du_github "a-v-medvedev" "v"
    if any_mode_is_set "bi"; then
        [ -f "$DNB_INSTALL_DIR/yaml-cpp.bin/include/yaml-cpp/yaml.h" ] || fatal "$pkg: installed yaml-cpp is required to build"
    fi
    local COMMANDS=""
    PARAMS="YAML_DIR=$DNB_INSTALL_DIR/yaml-cpp.bin WITH_SHARED_LIB=FALSE $argsparser_params"
    b_make "$COMMANDS" "clean"
    b_make "$COMMANDS" "$PARAMS"
    local FILES="argsparser/include/argsparser.h argsparser/libargsparser.a"
    i_direct_copy "$FILES" 
    FILES="argsparser/extensions"
    i_direct_copy "$FILES"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}" "$m"
    #i_save_this_pkg_record "$pkg" "${V}" "$m"
    return 0
}

function dnb_hypre() {
	#local pkg="hypre"
	#environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog "hypre" $* || return 0
    du_github "hypre-space" "v"
    u_apply_patches

    #if this_mode_is_set "u" "$m"; then
    #    N=$(ls -1 $DNB_DBSCRIPTSDIR/patch_${pkg}-${V}_*.diff 2>/dev/null | wc -l)
    #    if [ "$N" -gt "0" ]; then
    #        cd ${pkg}-${V}.src
    #        for p in ../$DNB_DBSCRIPTSDIR/patch_${pkg}-${V}_*.diff; do
    #    	set -x
    #            patch -p0 < $p
    #		set +x
    #        done
    #        cd ..
    #    fi
    #fi

    local OPTS=""
    OPTS="$OPTS CC=$MPICC"
    OPTS="$OPTS CFLAGS=-O3"
    OPTS="$OPTS CXX=$MPICXX"
    OPTS="$OPTS CXXFLAGS=-O3"
    OPTS="$OPTS --with-timing"
    OPTS="$OPTS --enable-shared"
    OPTS="$OPTS --without-openmp"
    OPTS="$OPTS --without-fei"
    OPTS="$OPTS --without-mli"
    OPTS="$OPTS --disable-fortran"
    bi_autoconf_make "cd src" "$OPTS"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}" "$m"
    return 0
}

function dnb_cppcgen() {
	#local pkg="cppcgen"
	#environment_check_specific "$pkg" || fatal "$pkg: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog "cppcgen" $* || return 0
    du_github "a-v-medvedev" "v"
    local COMMANDS=""
    local PARAMS="clean all"
    b_make "$COMMANDS" "$PARAMS"
    local FILES="distr/include distr/lib"
    i_direct_copy "$FILES"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}"  "$m"
    return 0
}

function dnb_numactl() {
	#local pkg="numactl"
	#environment_check_specific "$pkg" || fatal "$pkg: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog "numactl" $* || return 0
    du_github "numactl" "v"
    local COMMANDS=""
    local PARAMS="clean all"
    b_make "$COMMANDS" "$PARAMS"
    local FILES="libnuma.so.1 libnuma.so numa.h numaif.h"
    i_direct_copy "$FILES"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}"  "$m"
    return 0
}

function dnb_scotch() {
	#    local pkg="scotch"
	#environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog "scotch" $* || return 0
    du_gitlab "$pkg" "${pkg}-" "v" "gitlab.inria.fr"
    if this_mode_is_set "u"; then
        [ -e "${pkg}-${V}.src" ] && rm -rf "${pkg}-${V}.src"
        mv ${pkg}-v${V}.src ${pkg}-${V}.src
        cd ${pkg}-${V}.src
        cd src
        [ ! -e Makefile.inc ] && ln -s Make.inc/Makefile.inc.i686_pc_linux2.shlib Makefile.inc
        sed -i 's/LDFLAGS.*/& -lrt/;s/-DSCOTCH_PTHREAD//;s/-DCOMMON_PTHREAD/-DSCOTCH_DETERMINISTIC/' Makefile.inc
        cd $DNB_INSTALL_DIR
    fi
    local COMMANDS=""
    COMMANDS="cd src"
    local PARAMS=""
    PARAMS="$PARAMS CCS=$CC CCP=$MPICC CCD=$MPICC AR=$CC"
    b_make "$COMMANDS" "clean $PARAMS"
    b_make "$COMMANDS" "ptscotch $PARAMS"
    PARAMS="install installstub prefix=$DNB_INSTALL_DIR/${pkg}-${V}"
    i_make "$COMMANDS" "$PARAMS"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}" "$m"
    this_mode_is_set "i" && cp "${pkg}.bin/include_stub"/*.h "${pkg}.bin/include"
    return 0
}

function dnb_hdf5() {
    generic_prolog "hdf5" $* || return 0
    du_direct_download https://support.hdfgroup.org/ftp/HDF5/releases/${pkg}-${V%.*}/${pkg}-${V}/src/${pkg}-${V}.tar.gz "${pkg}-${V}.tar.gz" 
    local fortran_flag="--disable-fortran"
    is_set_to_true DNB_HDF5_ENABLE_FORTRAN && fortran_flag="--enable-fortran" 
    local COMMANDS="export H5_FCFLAGS=-fPIC"
    local OPTS="--enable-shared --disable-static $fortran_flag"
    bi_autoconf_make "$COMMANDS" "$OPTS" || fatal "hdf5: build failed."
    generic_epilog
    return 0
}

function dnb_CGNS() {
	#local pkg="CGNS"
	#environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog "CGNS" $* || return 0
    du_github "$pkg" "v"
    local COMMANDS="cd src"
    local OPTS="--enable-shared=all --without-fortran --with-zlib"
    [ -d "${DNB_INSTALL_DIR}/hdf5.bin" ] && OPTS="$OPTS --with-hdf5=${DNB_INSTALL_DIR}/hdf5.bin"
    bi_autoconf_make "$COMMANDS" "$OPTS"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}" "$m"
    return 0
}

function dnb_silo() {
	#local pkg="silo"
	#environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog $* || return 0

    local ret=0
    version_is_less "${V}" "4.11" || ret=$?

    if [ "${ret}" == "0" ]; then
        local dwnld_user="visit-dav"
        local dwnld_url="third-party"
        local visit_tag=$(github_get_latest_version_tag "$dwnld_user" "$dwnld_url")

        du_direct_download "https://github.com/$dwnld_user/$dwnld_url/releases/download/$visit_tag/${pkg}-${V}.tar.gz" ${pkg}-${V}.tar.gz
    else
        du_github "LLNL" ""
    fi

    if this_mode_is_set "u"; then
	local host_name=`hostname -s`
	rm -f ${pkg}-${V}.src/config-site/${host_name}.conf
    fi
    local COMMANDS=""
    local OPTS="--enable-shared --disable-fortran --disable-hzip --disable-fpzip"
    [ -d "${DNB_INSTALL_DIR}/hdf5.bin" ] && OPTS="$OPTS --with-hdf5=${DNB_INSTALL_DIR}/hdf5.bin/include,${DNB_INSTALL_DIR}/hdf5.bin/lib"
    bi_autoconf_make "$COMMANDS" "$OPTS"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}" "$m"
    return 0
}

function dnb_bazelisk() {
	#local pkg="bazelisk"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog "bazelisk" $* || return 0
    du_direct_download "https://github.com/bazelbuild/bazelisk/releases/download/v${V}/bazelisk-linux-amd64" bazelisk.exe
    i_direct_copy "bazelisk.exe"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}" "$m"
    if this_mode_is_set "i"; then
        cd ${pkg}-${V}
        [ -e "bazelisk" ] || ln -s bazelisk.exe bazelisk
        chmod +x bazelisk.exe
        cd $DNB_INSTALL_DIR
    fi
    return 0
}

# NOTE: bazel build system can't handle any other name of 'python' (i.e. 'python3' is not OK)
# One can make a symlink like: "ln -s /usr/bin/python3 /usr/bin/python" -- and this seems to be
# the only good option. The option: "export PYTHON_BIN_PATH=/usr/bin/python3" seems to work only
# partially
function dnb_tensorflow() {
	#local pkg="tensorflow"
	#environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
	#local m=$(get_field "$1" 2 "=")
	#local V=$(get_field "$2" 2 "=")
    generic_prolog "tensorflow" $* || return 0	
    local cache_dir=$DNB_INSTALL_DIR/_cache
    du_github "$pkg" "v"
    if this_mode_is_set "b"; then
        [ -e bazelisk.bin ] || fatal "bazelisk required to build tensorflow"
        cd ${pkg}-${V}.src
        local ram=$(free -m -w | awk '/available/ { for (i=1;i<=NF;i++) if ($i == "available") FLD=i+1; } /Mem:/ { print int($FLD / 8 * 7)  }')
        $DNB_INSTALL_DIR/bazelisk.bin/bazelisk clean
        rm -rf ~/.cache/bazel $cache_dir
        BAZELISK_HOME=$cache_dir $DNB_INSTALL_DIR/bazelisk.bin/bazelisk --output_user_root=$cache_dir build tensorflow:tensorflow_cc tensorflow:install_headers --jobs=$DNB_MAKE_PARALLEL_LEVEL --local_ram_resources=$ram
        cd $DNB_INSTALL_DIR
    fi
    [ -e ${pkg}-${V} ] && rm -rf ${pkg}-${V}
    local FILES="bazel-bin/tensorflow/libtensorflow_cc.so.$V bazel-bin/tensorflow/libtensorflow_framework.so.$V bazel-bin/tensorflow/include"
    i_direct_copy "$FILES"
    generic_epilog
    #i_make_binary_symlink "$pkg" "${V}"  "$m"
    if this_mode_is_set "i"; then
        cd "$pkg".bin
        ln -s libtensorflow_cc.so.$V libtensorflow_cc.so.2
        ln -s libtensorflow_cc.so.2 libtensorflow_cc.so
        ln -s libtensorflow_framework.so.$V libtensorflow_framework.so.2
        ln -s libtensorflow_framework.so.2 libtensorflow_framework.so
        cd $DNB_INSTALL_DIR
    fi
}

function dnb_netcdf-fortran() {
    generic_prolog "netcdf-fortran" $* || return 0
    du_github "Unidata" "v"
    local DNB_NETCDF_FORTRAN_NETCDF_C_PATH=${DNB_NETCDF_FORTRAN_NETCDF_C_PATH:-$DNB_INSTALL_DIR/netcdf-c.bin}
    local CMDS=""
    local OPTS=""
    OPTS="$OPTS FCFLAGS=-fPIC CFLAGS=-fPIC" 
    OPTS="$OPTS CPPFLAGS=-I$DNB_NETCDF_FORTRAN_NETCDF_C_PATH/include"
    OPTS="$OPTS -Wl,-L,$DNB_NETCDF_FORTRAN_NETCDF_C_PATH/lib,-rpath,$DNB_NETCDF_FORTRAN_NETCDF_C_PATH/lib"
    [ -v DNB_NETCDF_FORTRAN_FC ] && OPTS="$OPTS FC=$DNB_NETCDF_FORTRAN_FC"
    # NOTE: for newest gfortran versions: OPTS="$OPTS FCFLAGS+=${SPACE}-fallow-argument-mismatch"
    bi_autoconf_make "$CMDS" "$OPTS"
    generic_epilog
}

function dnb_netcdf-c() {
    generic_prolog "netcdf-c" $* || return 0	
    du_github "Unidata" "v"
    local DNB_NETCDF_C_HDF5_PATH=${DNB_NETCDF_C_HDF5_PATH:-$DNB_INSTALL_DIR/hdf5.bin}
    local OPTS=""
    OPTS="$OPTS CPPFLAGS=-I$DNB_NETCDF_C_HDF5_PATH/include"
    OPTS="$OPTS LDFLAGS=-L$DNB_NETCDF_C_HDF5_PATH/lib"
    OPTS="$OPTS --enable-parallel4"
    [ -v DNB_NETCDF_C_CC ] && OPTS="$OPTS CC=$DNB_NETCDF_C_CC"
    bi_autoconf_make "" "$OPTS"
    generic_epilog
}

