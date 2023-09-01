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
	local override_versions="${PACKAGE_VERSIONS}"
    local dependencies=""
    [ -z "$*" ] && dependencies="${PACKAGE_DEPS}"
    is_any_char_present "$*" ":" || dependencies="${PACKAGE_DEPS}"
    set -u
    local default_build_mode="$(get_default_build_mode)"
    
    local what_to_build=$(expand_mode_string "$*" "$PACKAGES" "$dependencies" "$default_build_mode")
    
	local was_installed=""

	local LIST1=$(print_all_exec_files "$TARGET_DIRS")
	#---
	for pkgver in $what_to_build; do
        local pkg=$(get_field "$pkgver" 1) 
		local mode=$(mode_for_pkg ${pkg} "$what_to_build")
		local version=$(version_for_pkg ${pkg} "${VERSIONS}" "${override_versions}" "${what_to_build}")
        if [ ! -z "$mode" ]; then
    		eval dnb_${pkg} mode=$mode version=$version
        fi
		this_mode_is_set "i" "$mode" && was_installed=1
	done
	if [ ! -z "$was_installed" ]; then
        check_bash_func_declared dnb_sandbox && dnb_sandbox
    fi
	#---
	LIST2=$(print_all_exec_files "$TARGET_DIRS" "$started")
	if [ ! -z "$was_installed" ]; then
		echo -ne "----------\nExecutables before build start: (unix-time, size, name)\n$LIST1"
		echo -ne "----------\nExecutables after build finish: (unix-time, size, name)\n$LIST2"
	fi
    rm -f .saved_pkg_records
    true
}

function du_direct_download() {
    local pkg="$1"
    local url="$2"
    local file="$3"
    local VER="$4"
    local m="$5"
    [ -z "$m" ] && return
    local target=${pkg}.dwn/$file
    if this_mode_is_set "d" "$m"; then
        [ -e "$target" ] && rm -rf "$target"
        [ ! -d "${pkg}.dwn" -a -e "${pkg}.dwn" ] && rm "${pkg}.dwn"
        mkdir -p "${pkg}.dwn"
        wget -nv -O "$target" "$url"
    fi
    if this_mode_is_set "u" "$m"; then
        [ -e "$target" ] || fatal "${pkg}: no downloaded target file: $target"
        [ -d ${pkg}-${VER}.src ] && rm -rf ${pkg}-${VER}.src
        [ -d ${pkg}-${VER} ] && rm -rf ${pkg}-${VER}
        local mime=$(file -b --mime-type "$target")
        if [ "$mime" == "application/gzip" -o "$mime" == "application/x-gzip" ]; then
            local DIR=$(tar tzf "$target" | head -n1 | awk -F/ '{print $1}')
            [ -e "$DIR"} ] && rm -rf "$DIR"
            tar zxf "$target"
            [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
            [ -e ${pkg}-${VER}.src ] && rm -rf ${pkg}-${VER}.src
            mv ${DIR} ${pkg}-${VER}.src
        else
            mkdir -p ${pkg}-${VER}.src
            mv $target ${pkg}-${VER}.src
        fi
    fi
}

function github_get_latest_version_tag() {
    local user="$1"
    local pkg="$2"
    T=$(mktemp)
    wget -q -O $T "https://api.github.com/repos/$user/$pkg/releases/latest"
    V=$(cat $T | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')
    rm $T
    echo $V
}

function du_github {
    local addr="$1"
    local pkg="$2"
    local pfx="$3"
    local VER="$4"
    local m="$5"
    [ -z "$m" ] && return
    local archive=${pkg}.dwn/${pkg}-${VER}.tar.gz
    if this_mode_is_set 'd' "$m" ; then
        [ -e "$archive" ] && rm -rf "$archive"
        [ ! -d "${pkg}.dwn" -a -e "${pkg}.dwn" ] && rm "${pkg}.dwn"
        mkdir -p "${pkg}.dwn"
        local url
        local XVER=$(get_field "${VER}" "1" "^")
        if [ "${XVER}" == "HEAD" ]; then
            local branch=master
            [ $(get_nfields "$VER" "^") == "2" ] && branch=$(get_field "${VER}" "2" "^")
            url="https://github.com/${addr}/${pkg}/archive/${branch}.tar.gz"
        else
            url="https://github.com/${addr}/${pkg}/archive/${pfx}${VER}.tar.gz"
        fi
        wget -nv -O "$archive" "$url"
    fi
    if this_mode_is_set 'u' "$m" ; then
        [ -e "$archive" ] || fatal "${pkg}: no downloaded archive file: $archive"
        local DIR=$(tar tzf "$archive" | head -n1 | sed s!/!!)
        [ -e "$DIR"} ] && rm -rf "$DIR"
        tar zxf "$archive"
        [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
        [ -e ${pkg}-${VER}.src ] && rm -rf ${pkg}-${VER}.src
        mv ${DIR} ${pkg}-${VER}.src
    fi
}

function du_gitlab {
    local addr="$1"
    local pkg="$2"
    local pfx="$3"
    local VER="$4"
    local m="$5"
    set +u
    local host="$6"
	local token="$7"
    local projectid="$8"
    set -u
    [ -z "$m" ] && return
    [ -z "$host" ] && host="gitlab.com"
    local archive=${pkg}.dwn/${pkg}-${VER}.tar.gz
    if this_mode_is_set 'd' "$m" ; then
        [ -e "$archive" ] && rm -rf "$archive"
        [ ! -d "${pkg}.dwn" -a -e "${pkg}.dwn" ] && rm "${pkg}.dwn"
        mkdir -p "${pkg}.dwn"
        local url=""
        local XVER=$(get_field "${VER}" "1" "^")
        local XBRANCH=$(get_field "${VER}" "2" "^")
        if [ ! -z "${token}" ]; then
            local sha=""
            [ "${XVER}" == "HEAD" ] || sha="?sha=${VER}" && sha="?sha=$XBRANCH"
            [ -z "$projectid" ] && projectid="${addr}%2F${pkg}"
            url="https://${host}/api/v4/projects/$projectid/repository/archive${sha}"
        else
            if [ "${XVER}" == "HEAD" ]; then
                local branch=master
                [ $(get_nfields "$VER" "^") == "2" ] && branch=$XBRANCH
                url="https://${host}/${addr}/${pkg}/-/archive/${branch}/${pfx}${branch}.tar.gz"
            else
                url="https://${host}/${addr}/${pkg}/-/archive/${VER}/${pfx}${VER}.tar.gz"
            fi
        fi
		local header=""
	    [ -z "${token}" ] || header="--header=PRIVATE-TOKEN:$token"
        wget -nv -O "$archive" $header "$url" 
    fi
    if this_mode_is_set 'u' "$m" ; then
        [ -e "$archive" ] || fatal "${pkg}: no downloaded archive file: $archive"
        local DIR=$(tar tzf "$archive" | head -n1 | sed s!/!!)
        [ -e ${DIR} ] && rm -rf ${DIR}
        tar zxf "$archive"
        [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
        [ -e ${pkg}-${VER}.src ] && rm -rf ${pkg}-${VER}.src
        mv ${DIR} ${pkg}-${VER}.src
    fi
}

function gitclone {
    local proj="$1"
    local addr="$2"
    local VER="$3"
    local branchname="$4"
    [ -d "$proj"-${VER}.src ] && rm -rf "$proj"-${VER}.src
    [ ! -z "$branchname" ] && branchopt="-b $branchname"
    git clone "$addr" $branchopt "$proj"-${VER}.src
}

function du_gitclone_recursive() {
    local pkg="$1"
    local addr="$2"
    local V="$3"
	local m="$4"
	if this_mode_is_set 'd' "$m" ; then
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
	if this_mode_is_set 'u' "$m" ; then
		[ -e "$pkg"-"$V".src ] || fatal "${pkg}-${V}.src directory not found, must be downloaded first."
	fi
}

function du_svn() { 
    local pkg="$1"
    local url="$2"
    local VER="$3" 
    local m="$4"
    mkdir -p "${pkg}.dwn"
    if this_mode_is_set 'd' "$m"; then
        cd "${pkg}.dwn" 
        svn --force export "$url" "$pkg"
        cd "$pkg" 
        if [ -d "$pkg" ]; then
            tar czf "../${pkg}-${VER}.tar.gz" "$pkg"
        else
            cd ..
            tar czf "${pkg}-${VER}.tar.gz" "$pkg"
        fi
        cd $INSTALL_DIR
    fi
    if this_mode_is_set 'u' "$m"; then
        local archive="${pkg}.dwn/${pkg}-${VER}.tar.gz"
        [ -e "$archive" ] || fatal "${pkg}: no downloaded archive file: $archive"
        local DIR=$(tar tzf "$archive" | head -n1 | sed s!/!!)
        [ "$DIR" == "." ] && fatal "malformed source tar archive: ${archive}."
        [ -e "$DIR" ] && rm -rf "$DIR"
        tar zxf "$archive"
        [ -d "$DIR" ] || fatal "${pkg}: error handling directory name in downloaded archive"
        [ -e "${pkg}-${VER}.src" ] && rm -rf "${pkg}-${VER}.src"
        mv "${DIR}" "${pkg}-${VER}.src"
    fi
}

function bi_cmake() {
    local proj="$1"
    local VER="$2"
    local cmpath="$3"
    local opts="$4"
    local m="$5"
    [ -z "$m" ] && return
    if this_mode_is_set 'b' "$m" ; then
        [ ! -e ${proj}-${VER}.src ] && fatal "${proj}: no source directory to build"
        cd ${proj}-${VER}.src
        [ -e build ] && rm -rf build
        mkdir -p build
        cd build
        cmake $opts -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR/${proj}-${VER} $cmpath
        make clean
        make -j${MAKE_PARALLEL_LEVEL}
        cd $INSTALL_DIR
    fi
    if this_mode_is_set 'i' "$m"; then
        [ ! -e ${proj}-${VER}.src ] && fatal "${proj}: no source directory to install from"
        cd ${proj}-${VER}.src
        [ ! -e build ] && fatal "${proj}: no build directory in source tree"
        cd build
        make install
        cd $INSTALL_DIR
    fi
}

function bi_autoconf_make() {
    local pkg="$1"
    local VER="$2"
    local commands="$3"
    local opts="$4"
    local m="$5"
    if this_mode_is_set 'b' "$m"; then
        [ ! -e ${pkg}-${VER}.src ] && fatal "${pkg}: no source directory to build"
        cd ${pkg}-${VER}.src
        [ ! -z "$commands" ] && eval "$commands"
	set -x
        ./configure $opts --prefix=$INSTALL_DIR/${pkg}-${VER}
	set +x
        make clean
        make -j $MAKE_PARALLEL_LEVEL
        cd $INSTALL_DIR
    fi
    if this_mode_is_set 'i' "$m"; then
        [ ! -e ${pkg}-${VER}.src ] && fatal "${pkg}: no source directory to install from"
        cd ${pkg}-${VER}.src
        [ ! -z "$commands" ] && eval "$commands"
        make install
        cd $INSTALL_DIR
    fi
}

function bi_make() {
    local proj="$1"
    local VER="$2"
    local COMMANDS="$3"
    local PARAMS="$4"
    local m="$5"
    if any_mode_is_set "bi" "$m"; then
        [ ! -e ${proj}-${VER}.src ] && fatal "${proj}: no source directory to build"
        cd ${proj}-${VER}.src
        eval "$COMMANDS"
        make -j $MAKE_PARALLEL_LEVEL $PARAMS
        cd $INSTALL_DIR
    fi
}

function b_make() {
    if this_mode_is_set "b" "$5"; then
        bi_make "$1" "$2" "$3" "$4" "$5"
    fi
}

function i_make() {
    if this_mode_is_set "i" "$5"; then
        bi_make "$1" "$2" "$3" "$4" "$5"
    fi
}

function i_make_binary_symlink() {
    local proj="$1"
    local VER="$2"
    local m="$3"
    if this_mode_is_set 'i' "$m"; then
	    rm -f $INSTALL_DIR/${proj}.bin
    	ln -s $INSTALL_DIR/${proj}-${VER} $INSTALL_DIR/${proj}.bin
        rm -f $INSTALL_DIR/${proj}.src
        ln -s $INSTALL_DIR/${proj}-${VER}.src $INSTALL_DIR/${proj}.src
    fi
}

function i_direct_copy() {
    local proj="$1"
    local VER="$2"
    local FILES="$3"
    local m="$4"
    if this_mode_is_set 'i' "$m"; then
        [ ! -e ${proj}-${VER}.src ] && fatal "${proj}: no source directory to install from"
        cd ${proj}-${VER}.src
        mkdir -p $INSTALL_DIR/${proj}-${VER}
        cp -r $FILES $INSTALL_DIR/${proj}-${VER}
        cd $INSTALL_DIR
    fi
}
