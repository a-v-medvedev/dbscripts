# dbscripts

A set of helper scripts for source code dependencies building (forked from https://gitlab.com/xamg/dbscripts)

You can add this repository as a submodule to your own repository.

Then you can add a `dnb.sh` script that is looking like:

```
[ -f ./env.sh ] && source ./env.sh || echo "WARNING: no env.sh file found"

BSCRIPTSDIR=./dbscripts

source $BSCRIPTSDIR/base.inc
source $BSCRIPTSDIR/funcs.inc
source $BSCRIPTSDIR/compchk.inc
source $BSCRIPTSDIR/envchk.inc
source $BSCRIPTSDIR/db.inc
source $BSCRIPTSDIR/apps.inc

function dnb_foobar() {
    local pkg="netcdf-fortran"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
	[ this_mode_is_set "d" ] && { do_actions_for_source_downloading; do_something_else_after_downloading; }
	[ this_mode_is_set "u" ] && { do_some_stuff_for_unpacking; do_something_else_after_unpacking; }
	[ this_mode_is_set "b" ] && { do_build_actions_in_unpacked_source_tree; }
	[ this_mode_is_set "i" ] && { do_install_steps; }
	i_make_binary_symlink "$pkg" "${V}" "$m"
}

function dnb_sandbox() {
    # optional final actions
}

PACKAGES="foobar yaml-cpp argsparser"
PACKAGE_DEPS="foobar:argsparser,yaml-cpp argsparser:yaml-cpp"
VERSIONS="foobar:1.0 argsparser:0.1 yaml-cpp:0.7.0"
TARGET_DIRS="foobar.bin yaml-cpp.bin argsparser.bin"

started=$(date "+%s")
echo "Download and build started at timestamp: $started."
environment_check_main || fatal "Environment is not supported, exiting"
dubi_main "$*"
finished=$(date "+%s")
echo "----------"
echo "Full operation time: $(expr $finished - $started) seconds."
```

Here we define a special function to download source code, unpack the downloaded archive (or prepare the cloned source code tree for build actions), build the source code and copy build results to a place where it is going to be used. The directory names are supposed to be: `${pkg}-${V}.dwn`, `${pkg}-${V}.src`, `${pkg}-${V}`. The `i_make_binary_symlink` subroutine makes a handy symlink for binary results: `${pkg}-${V} -> ${pkg}.bin`.

Besides the `i_make_binary_symlink` subroutine, there are many others that help to do some common actions. For example, if the source code is on github and is is built using standard autoconf/make process, the `dnb_foobar` function may look like:

```
function dnb_foobar() {
    local pkg="netcdf-fortran"
    environment_check_specific "$pkg" || fatal "${pkg}: environment check failed"
    local m=$(get_field "$1" 2 "=")
    local V=$(get_field "$2" 2 "=")
	du_github "somename" "pkg" "v" "$V" "$m"
    local OPTS=""
    OPTS="$OPTS CPPFLAGS=-I$INSTALL_DIR/something.bin/include"
    OPTS="$OPTS LDFLAGS=-L$INSTALL_DIR/something.bin/lib"
    OPTS="$OPTS --enable-some-feature"
    bi_autoconf_make "$pkg" "$V" "" "$OPTS" "$m"
    i_make_binary_symlink "$pkg" "${V}" "$m"
}

```

In these predefined functions (see db.inc), the first letters in name denote which of `d`, `u`, `b`, `i` build modes it handles. Build modes are just acronums from `download`, `build`, `unpack`, `install`.  

As for version signature, the `du_github` and similar functions understand versions as tag names or as branch names combined with changeset id, like: `foobar:master^HEAD` or `foobar:myfeature^0987fe4`.
