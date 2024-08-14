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

function dubi_main {
    touch .saved_pkg_records
    set +u
    local override_versions="${DNB_PACKAGE_VERSIONS}"
    local dependencies=""
    [ -z "$*" ] && dependencies="${DNB_PACKAGE_DEPS}"
    is_any_char_present "$*" ":" || dependencies="${DNB_PACKAGE_DEPS}"
    set -u
    local default_build_mode="$(get_default_build_mode)"
    local what_to_build=$(expand_mode_string "$*" "$DNB_PACKAGES" "$dependencies" "$default_build_mode")
    local was_installed=""

    echo ">> dbscripts: dubi_main:$what_to_build"

    local LIST1=$(print_all_exec_files "$DNB_TARGET_DIRS")
 #--- MAIN CYCLE
    for pkgver in $what_to_build; do
        local pkg=$(get_field "$pkgver" 1) 
        local mode=$(mode_for_pkg ${pkg} "$what_to_build")
        local version=$(version_for_pkg ${pkg} "${DNB_VERSIONS}" "${override_versions}" "${what_to_build}")
        if [ ! -z "$mode" ]; then
            echo ">> dbscripts: dubi_main: dnb_${pkg} mode=$mode version=$version"
            eval dnb_${pkg} mode=$mode version=$version
        fi
        this_mode_is_set "i" "$mode" && was_installed=1
    done
    if [ ! -z "$was_installed" ]; then
        check_bash_func_declared dnb_sandbox && echo ">> dbscripts: dubi_main: dnb_sandbox" && dnb_sandbox
    fi
 #-------------
    LIST2=$(print_all_exec_files "$DNB_TARGET_DIRS" "$started")
    if [ ! -z "$was_installed" ]; then
       echo -ne "----------\nExecutables before build start: (unix-time, size, name)\n$LIST1"
       echo -ne "----------\nExecutables after build finish: (unix-time, size, name)\n$LIST2"
    fi
    rm -f .saved_pkg_records
    return 0
}

function du_direct_download() {
    #local pkg="$==1"
    local url="$1"
    local file="$2"
    #local VER="$==4"
    #local m="$==5"
    [ -z "$m" ] && return
    local target=${pkg}.dwn/$file
    if this_mode_is_set "d"; then
        [ -e "$target" ] && rm -rf "$target"
        [ ! -d "${pkg}.dwn" -a -e "${pkg}.dwn" ] && rm "${pkg}.dwn"
        mkdir -p "${pkg}.dwn"
        wget -nv -O "$target" "$url"
    fi
    if this_mode_is_set "u"; then
        [ -e "$target" ] || fatal "${pkg}: no downloaded target file: $target"
        [ -d ${pkg}-${V}.src ] && rm -rf ${pkg}-${V}.src
        [ -d ${pkg}-${V} ] && rm -rf ${pkg}-${V}
        local mime=$(file -b --mime-type "$target")
        if [ "$mime" == "application/gzip" -o "$mime" == "application/x-gzip" ]; then
            local DIR=$(tar tzf "$target" | head -n1 | awk -F/ '{print $1}')
            [ -e "$DIR"} ] && rm -rf "$DIR"
            tar zxf "$target"
            [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
            [ -e ${pkg}-${V}.src ] && rm -rf ${pkg}-${V}.src
            mv ${DIR} ${pkg}-${V}.src
        else
            mkdir -p ${pkg}-${V}.src
            mv $target ${pkg}-${V}.src
        fi
    fi
    return 0
}

function github_get_latest_version_tag() {
    local user="$1"
    local pkg="$2"
    T=$(mktemp)
    wget -q -O $T "https://api.github.com/repos/$user/$pkg/releases/latest"
    V=$(cat $T | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')
    rm $T
    echo $V
    return 0
}

function du_github {
    local addr="$1"
    #local pkg="$==2"
    local pfx="$2"
    #local VER="$==4"
    #local m="$==5"
    [ -z "$m" ] && return
    local archive=${pkg}.dwn/${pkg}-${V}.tar.gz
    if this_mode_is_set 'd'; then
        [ -e "$archive" ] && rm -rf "$archive"
        [ ! -d "${pkg}.dwn" -a -e "${pkg}.dwn" ] && rm "${pkg}.dwn"
        mkdir -p "${pkg}.dwn"
        local url
        local XV=$(get_field "${V}" "1" "^")
        if [ "${XV}" == "HEAD" ]; then
            local branch=master
            [ $(get_nfields "$V" "^") == "2" ] && branch=$(get_field "${V}" "2" "^")
            url="https://github.com/${addr}/${pkg}/archive/${branch}.tar.gz"
        else
            url="https://github.com/${addr}/${pkg}/archive/${pfx}${V}.tar.gz"
        fi
        wget -nv -O "$archive" "$url"
    fi
    if this_mode_is_set 'u'; then
        [ -e "$archive" ] || fatal "${pkg}: no downloaded archive file: $archive"
        local DIR=$(tar tzf "$archive" | head -n1 | sed s!/!!)
        [ -e "$DIR"} ] && rm -rf "$DIR"
        tar zxf "$archive"
        [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
        [ -e ${pkg}-${V}.src ] && rm -rf ${pkg}-${V}.src
        mv ${DIR} ${pkg}-${V}.src
    fi
    return 0
}

function du_gitlab {
    local addr="$1"
    #local pkg="$==2"
    local pfx="$2"
    #local VER="$==4"
    #local m="$==5"
    set +u
    local host="$3"
    local token="$4"
    local projectid="$5"
    set -u
    [ -z "$m" ] && return
    [ -z "$host" ] && host="gitlab.com"
    local archive=${pkg}.dwn/${pkg}-${V}.tar.gz
    if this_mode_is_set 'd'; then
        [ -e "$archive" ] && rm -rf "$archive"
        [ ! -d "${pkg}.dwn" -a -e "${pkg}.dwn" ] && rm "${pkg}.dwn"
        mkdir -p "${pkg}.dwn"
        local url=""
        local XV=$(get_field "${V}" "1" "^")
        local XBRANCH=$(get_field "${V}" "2" "^")
        if [ ! -z "${token}" ]; then
            local sha=""
            [ "${XV}" == "HEAD" ] || sha="?sha=${V}" && sha="?sha=$XBRANCH"
            [ -z "$projectid" ] && projectid="${addr}%2F${pkg}"
            url="https://${host}/api/v4/projects/$projectid/repository/archive${sha}"
        else
            if [ "${XV}" == "HEAD" ]; then
                local branch=master
                [ $(get_nfields "$V" "^") == "2" ] && branch=$XBRANCH
                url="https://${host}/${addr}/${pkg}/-/archive/${branch}/${pfx}${branch}.tar.gz"
            else
                url="https://${host}/${addr}/${pkg}/-/archive/${V}/${pfx}${V}.tar.gz"
            fi
        fi
        local header=""
        [ -z "${token}" ] || header="--header=PRIVATE-TOKEN:$token"
        wget -nv -O "$archive" $header "$url" 
    fi
    if this_mode_is_set 'u'; then
        [ -e "$archive" ] || fatal "${pkg}: no downloaded archive file: $archive"
        local DIR=$(tar tzf "$archive" | head -n1 | sed s!/!!)
        [ -e ${DIR} ] && rm -rf ${DIR}
        tar zxf "$archive"
        [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
        [ -e ${pkg}-${V}.src ] && rm -rf ${pkg}-${V}.src
        mv ${DIR} ${pkg}-${V}.src
    fi
    return 0
}

function gitclone {
	#local proj="$==1"
    local addr="$1"
    #local VER="$==3"
    #local branchname="$==4"
    [ -d "$pkg"-${V}.src ] && rm -rf "$pkg"-${V}.src
    [ ! -z "$branchname" ] && branchopt="-b $branchname"
    git clone "$addr" $branchopt "$pkg"-${V}.src
    return 0
}

function du_gitclone_recursive() {
	#local pkg="$==1"
    local addr="$1"
    #local V="$==3"
    #local m="$==4"
    if this_mode_is_set 'd'; then
       [ -e "$pkg"-"$V".src ] && rm -rf "$pkg"-"$V".src
       mkdir -p "$pkg"-"$V".src
       cd "$pkg"-"$V".src
       local branch="$V"
       local XVER=$(get_field "${V}" "1" "^")
       if [ "${XVER}" == "HEAD" ]; then
           branch="master"
           [ $(get_nfields "${V}" "^") == "2" ] && branch=$(get_field "${V}" "2" "^")
       fi
       git clone --depth 1 --single-branch --branch "$branch" --recursive "${addr}" .
       cd ..
    fi
    if this_mode_is_set 'u'; then
        [ -e "$pkg"-"$V".src ] || fatal "${pkg}-${V}.src directory not found, must be downloaded first."
    fi
    return 0
}

function du_svn() { 
	#local pkg="$==1"
    local url="$1"
    #local VER="$==3" 
    #local m="$==4"
    mkdir -p "${pkg}.dwn"
    if this_mode_is_set 'd'; then
        cd "${pkg}.dwn" 
        svn --force export "$url" "$pkg"
        cd "$pkg" 
        if [ -d "$pkg" ]; then
            tar czf "../${pkg}-${V}.tar.gz" "$pkg"
        else
            cd ..
            tar czf "${pkg}-${V}.tar.gz" "$pkg"
        fi
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'u'; then
        local archive="${pkg}.dwn/${pkg}-${V}.tar.gz"
        [ -e "$archive" ] || fatal "${pkg}: no downloaded archive file: $archive"
        local DIR=$(tar tzf "$archive" | head -n1 | sed s!/!!)
        [ "$DIR" == "." ] && fatal "malformed source tar archive: ${archive}."
        [ -e "$DIR" ] && rm -rf "$DIR"
        tar zxf "$archive"
        [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
        [ -e "${pkg}-${V}.src" ] && rm -rf "${pkg}-${V}.src"
        mv "${DIR}" "${pkg}-${V}.src"
    fi
    return 0
}

function bi_cmake() {
	#local proj="$==1"
	#local VER="$==2"
    local cmpath="$1"
    local opts="$2"
    #local m="$==5"
    [ -z "$m" ] && return
    if this_mode_is_set 'b'; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to build"
        cd ${pkg}-${V}.src
        [ -e build ] && rm -rf build
        mkdir -p build
        cd build
        cmake $opts -DCMAKE_INSTALL_PREFIX=$DNB_INSTALL_DIR/${pkg}-${V} $cmpath
        make clean
        make -j${DNB_MAKE_PARALLEL_LEVEL}
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'i'; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to install from"
        cd ${pkg}-${V}.src
        [ ! -e build ] && fatal "${pkg}: no build directory in source tree"
        cd build
        make install
        cd $DNB_INSTALL_DIR
    fi
    return 0
}

function bi_autoconf_make() {
	#local pkg="$==1"
	#local VER="$==2"
    local commands="$1"
    local opts="$2"
    #local m="$==5"
    if this_mode_is_set 'b'; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to build"
        cd ${pkg}-${V}.src
        [ ! -z "$commands" ] && eval "$commands"
 set -x
        ./configure $opts --prefix=$DNB_INSTALL_DIR/${pkg}-${V}
 set +x
        make clean
        make -j $DNB_MAKE_PARALLEL_LEVEL
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'i'; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to install from"
        cd ${pkg}-${V}.src
        [ ! -z "$commands" ] && eval "$commands"
        make install
        cd $DNB_INSTALL_DIR
    fi
    return 0
}

function bi_make() {
	#    local proj="$==1"
	#local VER="$==2"
    local commands="$1"
    local params="$2"
    set +u
    local files="$3"
    set -u
    #local m="$==5"
    if any_mode_is_set "bi"; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to build"
        cd ${pkg}-${V}.src
        eval "$commands"
        make -j $DNB_MAKE_PARALLEL_LEVEL $params
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'i'; then
        [ -z "$files" ] || i_direct_copy "$files"
    fi
    return 0
}

function b_make() {
    if this_mode_is_set "b"; then
        bi_make "$1" "$2"
    fi
    return 0
}

function i_make() {
    if this_mode_is_set "i"; then
        set +u
        bi_make "$1" "$2" "$3"
	set -u
    fi
    return 0
}

function i_make_binary_symlink() {
	#local proj="$1"
	#local VER="$2"
	#local m="$3"
    if this_mode_is_set 'i'; then
        rm -f $DNB_INSTALL_DIR/${pkg}.bin
        ln -s $DNB_INSTALL_DIR/${pkg}-${V} $DNB_INSTALL_DIR/${pkg}.bin
        rm -f $DNB_INSTALL_DIR/${pkg}.src
        ln -s $DNB_INSTALL_DIR/${pkg}-${V}.src $DNB_INSTALL_DIR/${pkg}.src
    fi
    return 0
}

function i_direct_copy() { 
	#local proj="$==1"
	#local VER="$==2"
    local files="$1"
    #local m="$==4"
    set +u
    local flags="$2"
    set -u
    if this_mode_is_set 'i'; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to install from"
        cd ${pkg}-${V}.src
	[ "$flags" == "remove-before" ] && rm -rf $DNB_INSTALL_DIR/${pkg}-${V}
        mkdir -p $DNB_INSTALL_DIR/${pkg}-${V}
        cp -r $files $DNB_INSTALL_DIR/${pkg}-${V}
        cd $DNB_INSTALL_DIR
    fi
    return 0
}

function u_apply_patches() {
    if this_mode_is_set "u"; then
        N=$(ls -1 $DNB_DBSCRIPTSDIR/patch_${pkg}-${V}_*.diff 2>/dev/null | wc -l)
        if [ "$N" -gt "0" ]; then
            cd ${pkg}-${V}.src
            for p in ../$DNB_DBSCRIPTSDIR/patch_${pkg}-${V}_*.diff; do
               set -x
                patch -p0 < $p
               set +x
            done
            cd ..
        fi
    fi
    return 0
}

function generic_prolog() {
    export pkg="$1"
    shift
    export m=$(get_field "$1" 2 "=")
    export V=$(get_field "$2" 2 "=")
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    [ "$(cache_check_this_pkg_record $pkg $V $m)" == "cached" ] && { echo ">> dbscripts: package \"$pkg\" cached"; return 1; }
    return 0
}

function generic_epilog() {
    i_make_binary_symlink
    i_cache_save_this_pkg_record "$pkg" "$V" "$m"
    return 0
}

