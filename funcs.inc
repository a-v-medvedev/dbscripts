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

function num_files_by_mask() {
    local nf=$(ls -1 $1 2>/dev/null | wc -l)
    echo $nf
}

function fail_if_variable_not_defined() {
    for v in $*; do
        [ -v $v ] || fatal "the required variable \"$v\" is not defined."
       [ -z "${!v}" ] && fatal "the required variable \"$v\" is not set."
    done
}

function is_set_to_true() {
    local varname=$1
    set +u
    [ -z "${!varname}" -o "${!varname}" == 0 -o "${!varname}" == false -o "${!varname}" == FALSE -o "${!varname}" == OFF -o "${!varname}" == off ] && return 1
    set -u
    return 0
}

function beginswith() {
    case $1 in
        $2*) return 0;;
        *) return 1;;
    esac
}

function find_including_paths() {
    for i in $(seq 1 $#); do
        local p=$(eval echo \"\$$i\")
        [ -e $p ] && find $p/ -type f
    done
}

function find_excluding_paths() {
    local exclude_paths=""
    for i in $(seq 1 $#); do
        local p=$(eval echo \"\$$i\")
        exclude_paths=$(echo "$exclude_paths" -not -path "$p")
    done
    set -f
    find . $exclude_paths -type f
    set +f
}

function check_file_mask() {
	local filename="$1"
	local mask="$2"
	case "$1" in
	    $2) return 0; ;;
	    *) return 1; ;;
	esac
}

function is_static_lib() {
    local f="$1"
    if check_file_mask "$(basename $f)" "*.a" && [ "$(file -b "$f")" == "current ar archive" ]; then
        local non_object=false;
        ar t "$f" | while read n; do check_file_mask "$n" "*.o" || non_object=true; done
	[ "$non_object" == "false" ] && return 0;
    fi
    return 1
}

function is_elf() {
    local f="$1"
    [ $(file -b "$f" | awk '{print $1}') == "ELF" ] && return 0
    return 1
}

function ls_with_time() {
    for i in $*; do 
        ls -Bl1 --time-style="+%s" $i
    done
}

function print_all_exec_files() {
    local f=""
    set +u
    local paths="$1"
    local started_time="$2"
    set -u
    if [ -z "$paths" ]; then
        f=$(find_excluding_paths '*.src/*' './_*')
    else
        f=$(find_including_paths $paths)
    fi
    local allf=""
    [ -z "$f" ] && echo "" && return
    for i in $f; do 
	case "$i" in 
	    *.so|*.exe|*.dylib) is_elf "$i" && allf="$allf $i"; ;; 
	    *.a) is_static_lib "$i" && allf="$allf $i"; ;;
	    *) is_elf "$i" && allf="$allf $i"; ;;
	esac
    done
    ls_with_time $allf | awk -v T="$started_time" '{ if (T!="" && $6>=T) ISNEW=">>"; else ISNEW="--"; printf ISNEW " " $6 "\\t" "%15d" "\\t" $7 "\\n", $5 }'
}

function construct_replacement_command() {
    local from="$1"
    local to="$2"
    echo "s!$from!$to!;"
}

function generate_psubmit_opt() {
    set +u
    local dir=$1
    local suffix=$2
    [ -z "$dir" ] && dir="."
    local filename=$dir/psubmit.opt
    [ -z "$suffix" ] || filename=$dir/psubmit_$suffix.opt
    echo -n > $filename
    for i in QUEUE QUEUE_SUFFIX NODETYPE RESOURCE_HANDLING GENERIC_RESOURCES CONSTRAINT PPN NTH NGPUS INIT_COMMANDS INJOB_INIT_COMMANDS MPIEXEC BATCH WHITELIST BLACKLIST ACCOUNT RESERVATION COMMENT BEFORE AFTER TIME_LIMIT TARGET_BIN JOB_NAME; do
        local varname=PSUBMIT_OPT_$i
        [ -z "${!varname}" ] || echo $i=\'${!varname}\' >> $filename
    done
    set -u
    return 0
}

function i_generate_psubmit_opt() {
    set +u
    local suffix=$1
    set -u
    this_mode_is_set "i" && generate_psubmit_opt "$pkg-$V" $suffix
    return 0
}

function template_to_psubmitopts {
    local dir=$1
    local suffix="$2"
    local repl=""
    set +u
    repl=$(echo "${repl}" $(construct_replacement_command "__QUEUE__" "$PSUBMIT_OPTS_QUEUE"))
    repl=$(echo "${repl}" $(construct_replacement_command "__QUEUE_SUFFIX__" "$PSUBMIT_OPTS_QUEUE_SUFFIX"))
    repl=$(echo "${repl}" $(construct_replacement_command "__NODETYPE__" "$PSUBMIT_OPTS_NODETYPE"))
    repl=$(echo "${repl}" $(construct_replacement_command "__RESOURCE_HANDLING__" "$PSUBMIT_OPTS_RESOURCE_HANDLING"))
    repl=$(echo "${repl}" $(construct_replacement_command "__GENERIC_RESOURCES__" "$PSUBMIT_OPTS_GENERIC_RESOURCES"))
    repl=$(echo "${repl}" $(construct_replacement_command "__CONSTRAINT__" "$PSUBMIT_OPTS_CONSTRAINT"))
    repl=$(echo "${repl}" $(construct_replacement_command "__PPN__" "$PSUBMIT_OPTS_PPN"))
    repl=$(echo "${repl}" $(construct_replacement_command "__NTH__" "$PSUBMIT_OPTS_NTH"))
    repl=$(echo "${repl}" $(construct_replacement_command "__NGPUS__" "$PSUBMIT_OPTS_NGPUS"))
    repl=$(echo "${repl}" $(construct_replacement_command "__INIT_COMMANDS__" "$PSUBMIT_OPTS_INIT_COMMANDS"))
    repl=$(echo "${repl}" $(construct_replacement_command "__INJOB_INIT_COMMANDS__" "$PSUBMIT_OPTS_INJOB_INIT_COMMANDS"))
    repl=$(echo "${repl}" $(construct_replacement_command "__MPI_SCRIPT__" "$PSUBMIT_OPTS_MPIEXEC"))
    repl=$(echo "${repl}" $(construct_replacement_command "__BATCH_SCRIPT__" "$PSUBMIT_OPTS_BATCH"))
    repl=$(echo "${repl}" $(construct_replacement_command "__WHITELIST__" "$PSUBMIT_OPTS_WHITELIST"))
    repl=$(echo "${repl}" $(construct_replacement_command "__BLACKLIST__" "$PSUBMIT_OPTS_BLACKLIST"))
    repl=$(echo "${repl}" $(construct_replacement_command "__ACCOUNT__" "$PSUBMIT_OPTS_ACCOUNT"))
    repl=$(echo "${repl}" $(construct_replacement_command "__RESERVATION__" "$PSUBMIT_OPTS_RESERVATION"))
    repl=$(echo "${repl}" $(construct_replacement_command "__COMMENT__" "$PSUBMIT_OPTS_COMMENT"))
    repl=$(echo "${repl}" $(construct_replacement_command "__BEFORE__" "$PSUBMIT_OPTS_BEFORE"))
    repl=$(echo "${repl}" $(construct_replacement_command "__AFTER__" "$PSUBMIT_OPTS_AFTER"))
    repl=$(echo "${repl}" $(construct_replacement_command "__TIME_LIMIT__" "$PSUBMIT_OPTS_TIME_LIMIT"))
    repl=$(echo "${repl}" $(construct_replacement_command "__TARGET_BIN__" "$PSUBMIT_OPTS_TARGET_BIN"))
    repl=$(echo "${repl}" $(construct_replacement_command "__JOB_NAME__" "$PSUBMIT_OPTS_JOB_NAME"))
    if [ "$suffix" == "" ]; then
        cat psubmit.opt.TEMPLATE | sed "$repl" > $dir/psubmit.opt
    else
        cat psubmit_${suffix}.opt.TEMPLATE | sed "$repl" > $dir/psubmit_${suffix}.opt
    fi
    set -u
}

function get_default_build_mode() {
    set +u
    local default_build_mode="${DNB_DEFAULT_BUILD_MODE}"
    set -u
    [ -z "$default_build_mode" ] && default_build_mode=":dubi"
    echo "$default_build_mode"
}

function add_deps_iteration() {
    local packages="$1"
    local deps="$2"
    local m="$3"
    [ -z "$m" ] && m="$(get_default_build_mode)"
    local action=0
    for i in $deps; do
        p=$(get_field "$i" 1)
        pd=$(get_field "$i" 2)
        for n in $(seq 1 1 $(get_nfields "$pd" ",")); do
            j=$(get_field "$pd" "$n" ",")
            local dep_n=""
            local pkg_n=""
            local k=0 
            for pv in $packages; do
                k=$(expr "$k" \+ 1)
                local pkgname=$(get_field "$pv" 1)
                [ "$pkgname" == "$j" ] && dep_n=$k
                [ "$pkgname" == "$p" ] && pkg_n=$k
            done 
            [ -z "$pkg_n" ] && continue
            if [ -z "$dep_n" ]; then 
                packages="${j}${m} $packages";
                action=1
            elif [ "$dep_n" -ge "$pkg_n" ]; then 
                packages=$(swap_fields "$packages" "$pkg_n" "$dep_n")
                action=1
            fi
        done
    done
    echo "$packages"
    [ "$action" == "0" ] && return 0
    return 1
}

function add_deps() {
    local packages="$1"
    local deps="$2"
    local m="$3"
	local x=""
	local old_packages=$packages
	for x in $(seq 1 1 50); do
		set +e
		packages=$(add_deps_iteration "$packages" "$deps" "$m")
		if [ "$?" == "0" ]; then x=0; break; fi
		set -e
	done
	if [ "$x" == "0" ]; then
        [ "$packages" != "$old_packages" ] && warning "package list changed: $packages"
		echo $packages
	else
		fatal "Possible cyclic package dependencies."
	fi
}

function expand_mode_string {
    local allargs="$1"
    local dflt="$2"
    local deps="$3"
    local m="$4"
    local final=""
    [ -z "$m" ] && m="$(get_default_build_mode)"
    [ "$allargs" == "clean" ] && { echo " clean:i"; return 0; }
    [ -z "$allargs" ] && allargs="$m"
    [ ":" != $(get_first_char "$m") ] && fatal "wrong mode string"
    if [ ":" == $(get_first_char "$allargs") ]; then
        m="$allargs"
        for p in $dflt; do
            is_any_char_present "$p" ":" && final="$final $p" && continue
            final="$final ${p}${m}"
        done
    else
        for a in $allargs; do
            [ ":" == $(get_first_char "$a") ] && fatal "wrong mode string"
            [ "$a" == "sandbox" ] || beginswith "$a" "sandbox:" && fatal "package name sandbox is forbidden"
            [ "$a" == "clean" ] || beginswith "$a" "clean:" && fatal "clean can't be combined with anything or be a name of a package"
            is_any_char_present "$a" ":" && final="$final $a" && continue
            final="$final ${a}${m}"
        done
    fi
    [ -z "$deps" ] || final=$(add_deps "$final" "$deps" "$m")
    echo "$final"
    return 0
}

function mode_for_pkg {
    local pkg="$1"
    local bld="$2"
    for p in $bld; do
        if [ "$pkg" == $(get_field "$p" 1) ]; then
            get_field "$p" 2
            return
        fi
    done
    echo ""
    return 0
}

function version_for_pkg() {
    local pkg="$1"
    local defvers="$2"
    local cfgvers="$3"
    local bld="$4"
    [ "$pkg" == "clean" ] && { echo "0"; return 0; }
    local defm=$(mode_for_pkg ${pkg} "$defvers")
    local cfgm=$(mode_for_pkg ${pkg} "$cfgvers")
    local cmdm=""
    for p in $bld; do
        if [ "$pkg" == $(get_field "$p" 1) ]; then
            cmdm=$(get_field "$p" 3)
        fi
    done
    local version=""
    [ -z "$defm" ] && fatal "no default version for project: $pkg"
    [ -z "$cfgm" ] && cfgm="$defm"
    [ -z "$cmdm" ] && cmdm="$cfgm"
    version="$cmdm"
    echo "$version"
    return 0
}

function check_bash_func_declared() {
    if [ `type -t $1`"" != 'function' ]; then
        return 1
    fi
    return 0
}

