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


function i_cache_save_this_pkg_record() {
    local pkg="$1"
    local V="$2"
    local m="$3"
    [ -z "$m" ] && return 0
    local pkg_as_variable=$(echo "$pkg" | sed 's/-/_/g')_VERSION
    local record="local $pkg_as_variable"
    this_mode_is_set 'i' || { grep -q "$record" .saved_pkg_records && sed -i "/^$record/d" .saved_pkg_records; }
    this_mode_is_set 'i' || return 0
    local record_full="$record=$V"
    grep -q "$record_full" .saved_pkg_records || echo "$record_full" >> .saved_pkg_records
    return 0
}

function cache_check_this_pkg_record() {
    local pkg="$1"
    local V="$2"
    local m="$3"
    [ -z "$m" ] && return 0
    set +u
    [ -z "$DNB_SUPERIOR" ] && return 0
    set -u
    [ -e "$DNB_SUPERIOR/.saved_pkg_records" ] || return 0
    source "$DNB_SUPERIOR/.saved_pkg_records"
    local pkg_as_variable=$(echo "$pkg" | sed 's/-/_/g')_VERSION
    SAVED_VERSION=$(eval "echo \$$pkg_as_variable")
    if [ "$SAVED_VERSION" == "$V" ]; then
        if this_mode_is_set 'i'; then
            [ -e "$pkg.bin" ] && rm "$pkg.bin"
            set -x
            ln -s "$DNB_SUPERIOR/$pkg.bin" "$pkg.bin"
            set +x
            local record="local $pkg_as_variable"
            grep -q "$record" .saved_pkg_records && sed -i "/^$record/d" .saved_pkg_records
            local record_full="$record=$V"
            grep -q "$record_full" .saved_pkg_records || echo "local $pkg_as_variable=$V" >> .saved_pkg_records
        fi
        echo "cached"
        return 0
    fi
    return 0
}

