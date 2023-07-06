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

function verify_comp_type {
    local comp=$1
    local_ft=$2
    [ -z "$comp" ] && return 10 
    [ ! -x $(which $comp) ] && return 20
    local src=$(mktemp testsrcXXXXXX.$local_ft)
    local exe=$(mktemp testsrcXXXXXX.exe)

    cat > $src <<- EOM
    #include <stdio.h>
    int main(int argc, char **argv) 
    {
    #if defined (__INTEL_COMPILER)
        puts("intel"); 
        return 0;
    #endif
    #if (defined(__GNUC__) || defined(__GNUG__)) && !defined(__clang__) && !defined(__INTEL_COMPILER)
        puts("GNU"); 
        return 0;
    #endif
    #if defined(__clang__)
        puts("clang"); 
        return 0;
    #endif
        puts("UNKNOWN"); 
        return 30;
    }
EOM

    "$comp" -o $exe $src >& /dev/null
    local retcode=$?
    if [ "$retcode" != 0 ]; then
        >&2 echo ">> ERROR: verify_comp_type: compilation command returned non-zero code: $retcode."
        >&2 echo ">> verify_comp_type: Command was: $comp -o $exe $src"
        >&2 echo ">> verify_comp_type: Source file was: "
        >&2 echo ">> ---"
        >&2 cat $src
        >&2 echo ">> ---"
        rm -f $exe $src
        return 40
    fi
    local t=$(./$exe 2>/dev/null)
    retcode=$?
    [ ! -z "$t" ] && echo $t
    if [ "$retcode" != "0" ]; then
        >&2 echo ">> ERROR: verify_comp_type: compiler test source returned non-zero code: $retcode."
    fi
    rm -f $exe $src
    return $retcode
}

function align_version_to_3_fields() {
    echo $1 | awk -v RS=. 'BEGIN{n=0} /^[0-9]*[\n]*$/ {V[n++]=$1; next;} {ERR=1;} END {if (ERR!=1 || n>=2) { for (i=0;i<3;i++) if (V[i]=="") V[i]="0"; printf V[0] "." V[1] "." V[2] "\n" } }'
}

function get_version_in_gnu_style() {
	local cmp="$1"
	local ver_short=$($cmp -dumpversion 2>/dev/null) 
	local ver_full=$($cmp -dumpfullversion 2>/dev/null) 
    local nf_short=$(get_nfields "$ver_short")
    local nf_full=$(get_nfields "$ver_full")
    local ver=""
    if [ "$nf_short" -ge "$nf_full" ]; then
        ver=$ver_short
    else
        ver=$ver_full
    fi
	ver=$(align_version_to_3_fields $ver)
	echo $ver
}

function verify_comp_version {
    local type="$1"
    local cmp="$2"
    local ver=""
    local nf=""
    case "$type" in
      "GNU")
		ver=$(get_version_in_gnu_style "$cmp")
      ;;
      "intel")
		ver=$(get_version_in_gnu_style "$cmp")
      ;;
      "clang")
		ver=$(get_version_in_gnu_style "$cmp")
      ;;
      *)
		ver=$(get_version_in_gnu_style "$cmp")
		;;
    esac
    nf=$(get_nfields "$ver")
    if [ "3" != "$nf" ]; then echo "???"; return 1; fi
    echo "$ver"
    return 0
}


function get_minimal_version_for_type {
    local cpp_kind="$1"
    case "$cpp_kind" in
      "GNU") echo "$GNU_min_version"; return 0
      ;;
      "intel") echo "$INTEL_min_version"; return 0
      ;;
      "clang") echo "$CLANG_min_version"; return 0
      ;;
      *) return 1
    esac
}

function get_cc_cmp_version {
    local c="$1"
    local type=$(verify_comp_type "$c" c) && true
    if [ $? != 0 ]; then echo "ERR"; return 1; fi
    if [ "$type" == "" ]; then echo "ERR"; return 1; fi
    local v=$(verify_comp_version "$type" "$c") && true
    if [ "$?" != 0 ]; then echo "ERR"; return 1; fi
    echo $v
}

function get_cxx_cmp_version {
    local c="$1"
    local type=$(verify_comp_type "$c" cpp) && true
    if [ $? != 0 ]; then echo "ERR"; return 1; fi
    if [ "$type" == "" ]; then echo "ERR"; return 1; fi
    local v=$(verify_comp_version "$type" "$c") && true
    if [ "$?" != 0 ]; then echo "ERR"; return 1; fi
    echo $v
}
