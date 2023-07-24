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


function i_save_this_pkg_record() {
    local pkg="$1"
    local V="$2"
    local mode="$3"
    [ -z "$m" ] && return 0
    this_mode_is_set 'i' "$m" || return 0
    local pkg_as_variable=$(echo "$pkg" | sed 's/-/_/g')_VERSION
    local record="local $pkg_as_variable=$V"
    grep -q "$record" .saved_pkg_records || echo "local $pkg_as_variable=$V" >> .saved_pkg_records
    return 0
}

function check_this_pkg_record() {
    local pkg="$1"
    local V="$2"
    local mode="$3"
    [ -z "$m" ] && return 0
    set +u
    [ -z "$DNB_SUPERIOR" ] && return 0
    set -u
    [ -e "$DNB_SUPERIOR/.saved_pkg_records" ] || return 0
    source "$DNB_SUPERIOR/.saved_pkg_records"
    local pkg_as_variable=$(echo "$pkg" | sed 's/-/_/g')_VERSION
    SAVED_VERSION=$(eval "echo \$$pkg_as_variable")
    if [ "$SAVED_VERSION" == "$V" ]; then
        if this_mode_is_set 'i' "$m"; then
        [ -e "$pkg.bin" ] && rm "$pkg.bin"
            ln -s "$DNB_SUPERIOR/$pkg.bin" "$pkg.bin"
            local record="local $pkg_as_variable=$V"
            grep -q "$record" .saved_pkg_records || echo "local $pkg_as_variable=$V" >> .saved_pkg_records
        fi
        echo "cached"
        return 0
    fi
    return 0
}

function dnb_psubmit() {
    local pkg="psubmit"
    environment_check_specific "$pkg" || fatal "pkg: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    [ "$(check_this_pkg_record $pkg $V $m)" == "cached" ] && return 0
    du_github "a-v-medvedev" "psubmit" "v" "$V" "$m"
    if this_mode_is_set "i" "$m"; then
        local FILES=""
        cd ${pkg}-${V}.src
        FILES=$(ls -1 *.sh)
        cd $INSTALL_DIR
        i_direct_copy "$pkg" "$V" "$FILES" "$m"
        i_make_binary_symlink "$pkg" "${V}" "$m"
        i_save_this_pkg_record "$pkg" "${V}" "$m"
    fi
    return 0
}

function dnb_yaml-cpp() {
    local pkg="yaml-cpp"
    environment_check_specific "$pkg" || fatal "$pkg: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    [ "$(check_this_pkg_record $pkg $V $m)" == "cached" ] && return 0
    du_github "jbeder" "yaml-cpp" "yaml-cpp-" "$V" "$m"
    local OPTS=""
    OPTS="$OPTS -DYAML_BUILD_SHARED_LIBS=ON"
    OPTS="$OPTS -DYAML_CPP_BUILD_TESTS=OFF"
    OPTS="$OPTS -DYAML_CPP_BUILD_TOOLS=OFF"
    OPTS="$OPTS -DYAML_CPP_BUILD_CONTRIB=OFF"
    bi_cmake "$pkg" "$V" ".." "$OPTS" "$m"    
    i_make_binary_symlink "$pkg" "${V}" "$m"
    if this_mode_is_set "i" "$m"; then
        if [ -d "$pkg".bin/lib64 ]; then
            cd "$pkg".bin
            ln -s lib64 lib
            cd $INSTALL_DIR
        fi
    fi
    i_save_this_pkg_record "$pkg" "${V}" "$m"
    return 0
}

function dnb_argsparser() {
    local pkg="argsparser"
    environment_check_specific "$pkg" || fatal "$pkg: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    set +u
    local argsparser_params="$ARGSPARSER_PARAMS"
    set -u
    [ "$(check_this_pkg_record $pkg $V $m)" == "cached" ] && return 0
    du_github "a-v-medvedev" "argsparser" "v" "$V" "$m"
    if any_mode_is_set "bi" "$m"; then
        [ -f "$INSTALL_DIR/yaml-cpp.bin/include/yaml-cpp/yaml.h" ] || fatal "$pkg: installed yaml-cpp is required to build"
    fi
    local COMMANDS=""
    PARAMS="YAML_DIR=$INSTALL_DIR/yaml-cpp.bin $argsparser_params"
    b_make "$pkg" "$V" "$COMMANDS" "clean" "$m"
    b_make "$pkg" "$V" "$COMMANDS" "$PARAMS" "$m"
    local FILES="argsparser/include/argsparser.h argsparser/libargsparser.a"
    i_direct_copy "$pkg" "$V" "$FILES" "$m"
    [ -f "${pkg}-${V}.src/argsparser/cygargsparser.dll" ] && FILES="argsparser/cygargsparser.dll argsparser/libargsparser.dll.a" 
    [ -f "${pkg}-${V}.src/argsparser/libargsparser.so" ] && FILES="argsparser/libargsparser.so"
    i_direct_copy "$pkg" "$V" "$FILES" "$m"
    FILES="argsparser/extensions"
    i_direct_copy "$pkg" "$V" "$FILES" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
    i_save_this_pkg_record "$pkg" "${V}" "$m"
    return 0
}

function dnb_hypre() {
    local pkg="hypre"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_github "hypre-space" "hypre" "v" "$V" "$m"
    if this_mode_is_set "u" "$m"; then
        N=$(ls -1 $BSCRIPTSDIR/patch_${pkg}-${V}_*.diff 2>/dev/null | wc -l)
        if [ "$N" -gt "0" ]; then
            cd ${pkg}-${V}.src
            for p in ../$BSCRIPTSDIR/patch_${pkg}-${V}_*.diff; do
        	set -x
                patch -p0 < $p
		set +x
            done
            cd ..
        fi
    fi
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
    bi_autoconf_make "$pkg" "$V" "cd src" "$OPTS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
    return 0
}

function dnb_cppcgen() {
    local pkg="cppcgen"
    environment_check_specific "$pkg" || fatal "$pkg: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_github "a-v-medvedev" "cppcgen" "v" "$V" "$m"
    local COMMANDS=""
    local PARAMS="clean all"
    b_make "$pkg" "$V" "$COMMANDS" "$PARAMS" "$m"
    local FILES="distr/include distr/lib"
    i_direct_copy "$pkg" "$V" "$FILES" "$m"
    i_make_binary_symlink "$pkg" "${V}"  "$m"
    return 0
}

function dnb_numactl() {
    local pkg="numactl"
    environment_check_specific "$pkg" || fatal "$pkg: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_github "numactl" "numactl" "v" "$V" "$m"
    local COMMANDS=""
    local PARAMS="clean all"
    b_make "$pkg" "$V" "$COMMANDS" "$PARAMS" "$m"
    local FILES="libnuma.so.1 libnuma.so numa.h numaif.h"
    i_direct_copy "$pkg" "$V" "$FILES" "$m"
    i_make_binary_symlink "$pkg" "${V}"  "$m"
    return 0
}

function dnb_scotch() {
    local pkg="scotch"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_gitlab "$pkg" "$pkg" "${pkg}-" "v$V" "$m" "gitlab.inria.fr"
    if this_mode_is_set "u" "$m"; then
        [ -e "${pkg}-${V}.src" ] && rm -rf "${pkg}-${V}.src"
        mv ${pkg}-v${V}.src ${pkg}-${V}.src
        cd ${pkg}-${V}.src
        cd src
        [ ! -e Makefile.inc ] && ln -s Make.inc/Makefile.inc.i686_pc_linux2.shlib Makefile.inc
        sed -i 's/LDFLAGS.*/& -lrt/;s/-DSCOTCH_PTHREAD//;s/-DCOMMON_PTHREAD/-DSCOTCH_DETERMINISTIC/' Makefile.inc
        cd $INSTALL_DIR
    fi
    local COMMANDS=""
    COMMANDS="cd src"
    local PARAMS=""
    PARAMS="$PARAMS CCS=$CC CCP=$MPICC CCD=$MPICC AR=$CC"
    b_make "scotch" "${V}" "$COMMANDS" "clean $PARAMS" "$m"
    b_make "scotch" "${V}" "$COMMANDS" "ptscotch $PARAMS" "$m"
    PARAMS="install installstub prefix=$INSTALL_DIR/${pkg}-${V}"
    i_make "scotch" "${V}" "$COMMANDS" "$PARAMS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
    this_mode_is_set "i" "$m" && cp "${pkg}.bin/include_stub"/*.h "${pkg}.bin/include"
    return 0
}

function dnb_hdf5() {
    local pkg="hdf5"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_direct_download "$pkg" https://support.hdfgroup.org/ftp/HDF5/releases/${pkg}-${V%.*}/${pkg}-${V}/src/${pkg}-${V}.tar.gz "${pkg}-${V}.tar.gz" "$V" "$m"
    local COMMANDS=""
    local OPTS="--enable-shared --disable-static --disable-fortran"
    bi_autoconf_make "$pkg" "$V" "$COMMANDS" "$OPTS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
    return 0
}

function dnb_CGNS() {
    local pkg="CGNS"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_github "$pkg" "$pkg" v "$V" "$m"
    local COMMANDS="cd src"
    local OPTS="--enable-shared=all --without-fortran --with-zlib"
    [ -d "${INSTALL_DIR}/hdf5.bin" ] && OPTS="$OPTS --with-hdf5=${INSTALL_DIR}/hdf5.bin"
    bi_autoconf_make "$pkg" "$V" "$COMMANDS" "$OPTS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
    return 0
}

function dnb_silo() {
    local pkg="silo"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")

    local ret=0
    version_is_less "${V}" "4.11" || ret=$?

    if [ "${ret}" == "0" ]; then
        local dwnld_user="visit-dav"
        local dwnld_url="third-party"
        local visit_tag=$(github_get_latest_version_tag "$dwnld_user" "$dwnld_url")

        du_direct_download "$pkg" "https://github.com/$dwnld_user/$dwnld_url/releases/download/$visit_tag/${pkg}-${V}.tar.gz" ${pkg}-${V}.tar.gz "$V" "$m"
    else
        du_github "LLNL" "$pkg" "" "$V" "$m"
    fi

    if this_mode_is_set "u" "$m"; then
	local host_name=`hostname -s`
	rm -f ${pkg}-${V}.src/config-site/${host_name}.conf
    fi
    local COMMANDS=""
    local OPTS="--enable-shared --disable-fortran --disable-hzip --disable-fpzip"
    [ -d "${INSTALL_DIR}/hdf5.bin" ] && OPTS="$OPTS --with-hdf5=${INSTALL_DIR}/hdf5.bin/include,${INSTALL_DIR}/hdf5.bin/lib"
    bi_autoconf_make "$pkg" "$V" "$COMMANDS" "$OPTS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
    return 0
}

function dnb_bazelisk() {
    local pkg="bazelisk"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_direct_download "$pkg" "https://github.com/bazelbuild/bazelisk/releases/download/v${V}/bazelisk-linux-amd64" bazelisk.exe "$V" "$m"
    i_direct_copy "$pkg" "$V" "bazelisk.exe" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
    if this_mode_is_set "i" "$m"; then
        cd ${pkg}-${V}
        [ -e "bazelisk" ] || ln -s bazelisk.exe bazelisk
        chmod +x bazelisk.exe
        cd $INSTALL_DIR
    fi
    return 0
}

# NOTE: bazel build system can't handle any other name of 'python' (i.e. 'python3' is not OK)
# One can make a symlink like: "ln -s /usr/bin/python3 /usr/bin/python" -- and this seems to be
# the only good option. The option: "export PYTHON_BIN_PATH=/usr/bin/python3" seems to work only
# partially
function dnb_tensorflow() {
    local pkg="tensorflow"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    local cache_dir=$INSTALL_DIR/_cache
    du_github "$pkg" "$pkg" "v" "$V" "$m"
    if this_mode_is_set "b" "$m"; then
        [ -e bazelisk.bin ] || fatal "bazelisk required to build tensorflow"
        cd ${pkg}-${V}.src
        local ram=$(free -m -w | awk '/available/ { for (i=1;i<=NF;i++) if ($i == "available") FLD=i+1; } /Mem:/ { print int($FLD / 8 * 7)  }')
        $INSTALL_DIR/bazelisk.bin/bazelisk clean
        rm -rf ~/.cache/bazel $INSTALL_DIR/_cache
        BAZELISK_HOME=$cache_dir $INSTALL_DIR/bazelisk.bin/bazelisk --output_user_root=$cache_dir build tensorflow:tensorflow_cc tensorflow:install_headers --jobs=$MAKE_PARALLEL_LEVEL --local_ram_resources=$ram
		cd $INSTALL_DIR
    fi
    [ -e ${pkg}-${V} ] && rm -rf ${pkg}-${V}
    local FILES="bazel-bin/tensorflow/libtensorflow_cc.so.$V bazel-bin/tensorflow/libtensorflow_framework.so.$V bazel-bin/tensorflow/include"
    i_direct_copy "$pkg" "$V" "$FILES" "$m"
    i_make_binary_symlink "$pkg" "${V}"  "$m"
	if this_mode_is_set "i" "$m"; then
		cd "$pkg".bin
        ln -s libtensorflow_cc.so.$V libtensorflow_cc.so.2
        ln -s libtensorflow_cc.so.2 libtensorflow_cc.so
        ln -s libtensorflow_framework.so.$V libtensorflow_framework.so.2
        ln -s libtensorflow_framework.so.2 libtensorflow_framework.so
		cd $INSTALL_DIR
	fi
}

function dnb_netcdf-fortran() {
    local pkg="netcdf-fortran"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_github "Unidata" "netcdf-fortran" "v" "$V" "$m"
    local OPTS=""
    OPTS="$OPTS CPPFLAGS=-I$INSTALL_DIR/netcdf-c.bin/include"
#   for newest gfortran versions: OPTS="$OPTS FCFLAGS=-fallow-argument-mismatch""
    local CMDS="export LDFLAGS=\"-L$INSTALL_DIR/netcdf-c.bin/lib -Wl,-rpath,$INSTALL_DIR/netcdf-c.bin/lib\""
    bi_autoconf_make "$pkg" "$V" "$CMDS" "$OPTS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
}

function dnb_netcdf-c() {
    local pkg="netcdf-c"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
    du_github "Unidata" "netcdf-c" "v" "$V" "$m"
    local OPTS=""
    OPTS="$OPTS CPPFLAGS=-I$INSTALL_DIR/hdf5.bin/include"
    OPTS="$OPTS LDFLAGS=-L$INSTALL_DIR/hdf5.bin/lib"
    OPTS="$OPTS --enable-parallel4"
    bi_autoconf_make "$pkg" "$V" "" "$OPTS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
}
	
