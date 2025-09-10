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

    echo ">> dbscripts: dubi_main: actions:$what_to_build"

    local LIST1=$(print_all_exec_files "$DNB_TARGET_DIRS")
 #--- MAIN CYCLE
    for pkgver in $what_to_build; do
        local pkg=$(get_field "$pkgver" 1) 
        local mode=$(mode_for_pkg ${pkg} "$what_to_build")
        local version=$(version_for_pkg ${pkg} "${DNB_VERSIONS}" "${override_versions}" "${what_to_build}")
        if [ ! -z "$mode" ]; then
            echo ">> dbscripts: dubi_main: call: dnb_${pkg} mode=$mode version=$version"
            eval dnb_${pkg} mode=$mode version=$version
        fi
        [ "$pkg" == "clean" ] && { rm -f .saved_pkg_records; return 0; }
        this_mode_is_set "i" "$mode" && was_installed=1
    done
    if [ ! -z "$was_installed" ]; then
        check_bash_func_declared dnb_sandbox && echo ">> dbscripts: dubi_main: call: dnb_sandbox" && m=i && dnb_sandbox
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


function patch_single_file() {
    local filetopatch="$1"
    local ptch="$2"
    if [ -e "$filetopatch" -a -e "$ptch" ]; then
        echo ">> dbscripts: applying $(basename $ptch) to $filetopatch:"
        patch --dry-run -s -N -i $ptch $filetopatch && patch -i $ptch $filetopatch || true
    fi
    return 0
}

function u_patch_single_file() {
    this_mode_is_set "u" && patch_single_file "$1" "$2" 
    return 0
}

function i_patch_single_file() {
    this_mode_is_set "i" && patch_single_file "$1" "$2" 
    return 0
}

function u_apply_patches() {
    set +u
    local directory=$1
    set -u    
    [ -z "$directory" ] && directory=$DNB_DBSCRIPTSDIR
    if beginswith "$directory" "/"; then
        fatal "u_apply_patches: directory name containing patches must be relative to DNB root."
    fi
    directory="$DNB_INSTALL_DIR/$directory"
    [ -d "$directory" ] || fatal "u_apply_patches: directory doesn't exist: $directory"
    if this_mode_is_set "u"; then
        n1=$(num_files_by_mask $directory/patch_${pkg}-${V}_*.diff)
        n2=$(num_files_by_mask $directory/patch_${pkg}-ANY_*.diff)
        [ "$n1" == 0 -a "$n2" == 0 ] && return 0
        cd ${pkg}-${V}.src
        for p in $directory/patch_${pkg}-${V}_*.diff $directory/patch_${pkg}-ANY_*.diff; do
            if [ -e "$p" ]; then
                echo ">> dbscripts: applying $(basename $p):"
                patch --dry-run -s -N -p1 -i $p && patch -p1 -i $p || true
            fi
        done
        cd $DNB_INSTALL_DIR
    fi
    return 0
}


function u_make_source_symlink() {
    if this_mode_is_set 'u'; then	
        cd $DNB_INSTALL_DIR	
	rm -f ${pkg}.src
	ln -s ${pkg}-${V}.src ${pkg}.src
    fi
}

function du_direct_download() {
    local url="$1"
    local file="$2"
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
    u_make_source_symlink
    return 0
}

function github_get_latest_version_tag() {
    local user="$1"
    local T=$(mktemp)
    wget -q -O $T "https://api.github.com/repos/$user/$pkg/releases/latest"
    local version=$(cat $T | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')
    rm $T
    echo $version
    return 0
}

function du_github {
    local addr="$1"
    set +u
    local pfx="$2"
    local project="$3"
    set -u
    [ -z "$m" ] && return
    [ -z "$project" ] && project="${pkg}"
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
            url="https://github.com/${addr}/${project}/archive/${branch}.tar.gz"
        else
            url="https://github.com/${addr}/${project}/archive/${pfx}${V}.tar.gz"
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
    u_make_source_symlink
    return 0
}

function du_gitlab {
    local addr="$1"
    local pfx="$2"
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
    u_make_source_symlink
    return 0
}

function du_gitclone() {
    local addr="$1"
    set +u
    local flags="$2"
    local branch="$3"
    set -u
    if this_mode_is_set 'd'; then
       [ -e "$pkg"-"$V".src ] && rm -rf "$pkg"-"$V".src
       mkdir -p "$pkg"-"$V".src
       cd "$pkg"-"$V".src
       if [ -z "$branch" ]; then
         branch="$V"
         local XVER=$(get_field "${V}" "1" "^")
         if [ "${XVER}" == "HEAD" ]; then
             branch="master"
             [ $(get_nfields "${V}" "^") == "2" ] && branch=$(get_field "${V}" "2" "^")
         fi
       fi
       local recursive=""
       [ "$flags" == "recursive" ] && recursive="--recursive"
       git clone --depth 1 --single-branch --branch "$branch" $recursive "${addr}" .
       cd ..
    fi
    if this_mode_is_set 'u'; then
        [ -e "$pkg"-"$V".src ] || fatal "${pkg}-${V}.src directory not found, must be downloaded first."
    fi
    u_make_source_symlink
    return 0
}

function du_git_embedded() {
    local subdir="src"
    set +u
    [ -z "$1" ] || subdir="$1"
    set -u
    if this_mode_is_set d; then
        if [ "$V" != "CURRENT" ]; then
            local changes=$(git status -suno)
            [ -z "$changes" ] || fatal "there are local changes, can't switch code version."
            git checkout "$V" || fatal "error when checking out the branch or tag: $V."
        fi
        [ -d "$subdir" ] || fatal "embedded git option for a project assumes that all project source code is placed in $subdir subdirectory, but it missing for this code version."
    fi
    if this_mode_is_set u; then
        [ -e ${pkg}-${V}.src ] || ln -s "$subdir" ${pkg}-${V}.src
    fi
    return 0
}

function du_svn() { 
    local url="$1"
    mkdir -p "${pkg}.dwn"
    if this_mode_is_set 'd'; then
        cd "${pkg}.dwn" 
        svn --force -q export "$url" "$pkg"
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
    u_make_source_symlink
    return 0
}

function bi_cmake() {
    local cmpath="$1"
    local opts="$2"
    set +u 
    local commands="$3"
    set -u
    [ -z "$m" ] && return
    local cmake_binary=cmake
    [ -v DNB_CMAKE_BINARY ] && cmake_binary="$DNB_CMAKE_BINARY"
    if this_mode_is_set 'b'; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to build"
        cd ${pkg}-${V}.src
        [ -e build ] && rm -rf build
        mkdir -p build
        cd build
        $cmake_binary $opts -DCMAKE_INSTALL_PREFIX=$DNB_INSTALL_DIR/${pkg}-${V} $cmpath
        [ ! -z "$commands" ] && eval "$commands"
        make clean
        make -j${DNB_MAKE_PARALLEL_LEVEL}
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'i'; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to install from"
        cd ${pkg}-${V}.src
        [ ! -e build ] && fatal "${pkg}: no build directory in source tree"
        cd build
        [ ! -z "$commands" ] && eval "$commands"
        make install
        cd $DNB_INSTALL_DIR
    fi
    return 0
}

function bi_autoconf_make() {
    local commands="$1"
    local opts="$2"
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
    local commands="$1"
    local params="$2"
    set +u
    local files="$3"
    set -u
    if this_mode_is_set "b"; then
        [ ! -e ${pkg}-${V}.src ] && fatal "${pkg}: no source directory to build"
        cd ${pkg}-${V}.src
        eval "$commands" || { cd $DNB_INSTALL_DIR; return 1; }
        make -j $DNB_MAKE_PARALLEL_LEVEL $params || { cd $DNB_INSTALL_DIR; return 1; }
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
    if this_mode_is_set 'i'; then
        cd $DNB_INSTALL_DIR
        rm -f ${pkg}.bin
        ln -s ${pkg}-${V} ${pkg}.bin
    fi
    return 0
}

function i_direct_copy() { 
    local files="$1"
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

## -- mksandbox
function i_mksandbox() {
    local sandbox_dir=sandbox
    set +u
    [ -z "$1" ] || sandbox_dir=$1
    set -u
    mkdir -p $sandbox_dir
    cd $sandbox_dir
    if [ -v DNB_SANDBOX_SUBDIR ]; then
        [ "$DNB_SANDBOX_SUBDIR" == "." ] || mkdir -p "$DNB_SANDBOX_SUBDIR" && cd "$DNB_SANDBOX_SUBDIR"
    fi
    export DNB_SANDBOX=$PWD
    cd $DNB_INSTALL_DIR
}

## -- copyscripts
function i_copy_scripts() {
    for mask in "$DNB_INSTALL_DIR/scripts/*.sh" "$DNB_INSTALL_DIR/scripts/generic/*.sh" "$DNB_INSTALL_DIR/scripts/generic/$DNB_MACHINE_SCRIPTS_FOLDER/*.sh"; do
        if [ "$(num_files_by_mask $mask)" != 0 ]; then
            for i in $mask; do
                case $i in
                */scalability_*) ;;   
                *) ln -rsf $i .;;
                esac 
            done
        fi
    done
}

## -- copyscalscripts
function i_copy_scal_scripts() {
    for mask in $DNB_INSTALL_DIR/scripts/generic/scalability_*.*; do
        if [ "$(num_files_by_mask $mask)" != 0 ]; then
            for i in $mask; do
                ln -rsf $i .
            done
        fi
    done
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
    unset pkg V m
    return 0
}

