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

function fatal() {
	local str="$1"
	1>&2 echo "FATAL: $str"
	exit 1
}

function warning() {
    local str="$1"
    1>&2 echo "WARNING: $str"
}

function info() {
    local str="$1"
    1>&2 echo "$str"
}

function get_field() {
    local str="$1"
    local n="$2"
    set +u
    local delim="$3"
    set -u
    [ -z "$delim" ] && delim=":"
    echo "$str" | cut -d$delim -f$n
}

function get_nfields()
{
    local str="$1"
    local delim="$2"
    [ -z "$delim" ] && delim="."
    local nfields=$(echo $str | awk -F"$delim" '{print NF}')
    echo $nfields
}

function swap_fields() {
    local packages="$1"
    local pn=$2
    local dn=$3
    set +u
    local delim="$4"
    set -u
    if [ "$delim" == "" ]; then delim=" "; fi
    echo "$packages" | awk -v pn=$pn -v dn=$dn -F "$delim" '{ OFS=FS; x=$pn; $pn=$dn; $dn=x; print }'
}

function get_num_chars() {
    local str="$1"
    echo -n "$str" | wc -c
}

function get_first_char() {
    local str="$1"
    echo ${str:0:1}
}

function is_any_char_present() {
    local str="$1"
    local c="$2"
    echo -n "$str" | grep -q "[$c]" && return 0
    return 1
}

function this_mode_is_set {
    local f="$1"
    local m="$2"
    [ -z "$m" ] && return 1
    [  "1" != $(get_num_chars "$f") ] && echo "is_flag_set: wrong args" && exit 1
    is_any_char_present "$m" "$f" && return 0
    return 1
}

function any_mode_is_set {
    local f=$1
    local m=$2
    [ -z "$m" ] && return 1
    [  "1" -gt $(get_num_chars "$f") ] && echo "any_flag_set: wrong args" && exit 1
    is_any_char_present "$m" "$f" && return 0
    return 1
}

function get_csum() {
    local file="$1"
    md5sum < "$file" 2> /dev/null
}

function get_uplevel_dir() {
    echo $(dirname $(dirname "$1"))
}

function version_is_less {
    local v1=$(echo "$1" | awk -F. '{print $1*100000000+$2*10000+$3}')
    local v2=$(echo "$2" | awk -F. '{print $1*100000000+$2*10000+$3}')
    [ "$v1" -lt "$v2" ] && return 0 || return 1
}

function set_variable_if_not_defined() {
    local varname="$1"
    local value="$2"
    local x=$(eval echo \$$varname)
    [ -z "$x" ] && eval export $varname=$value
}

function check_if_bash_func_declared {
	local v="0"
    [ "$(type -t $1)" == "function" ] && v="1"
    eval ${1}_is_declared=$v
}

