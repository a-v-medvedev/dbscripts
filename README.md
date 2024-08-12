# dbscripts

A set of helper scripts for source code dependencies building (forked from https://gitlab.com/xamg/dbscripts)

## Usage method

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
    local pkg="foobar"
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

PACKAGES="foobar yaml-cpp argsparser"     # Mandatory
PACKAGE_DEPS="foobar:argsparser,yaml-cpp argsparser:yaml-cpp"    # Optional (not recommended)
VERSIONS="foobar:1.0 argsparser:0.1 yaml-cpp:0.7.0"     # Mandatory
TARGET_DIRS="foobar.bin yaml-cpp.bin argsparser.bin"    # Optional (recommended)

started=$(date "+%s")
echo "Download and build started at timestamp: $started."
environment_check_main || fatal "Environment is not supported, exiting"
dubi_main "$*"
finished=$(date "+%s")
echo "----------"
echo "Full operation time: $(expr $finished - $started) seconds."
```

Here we define a special function to download source code, unpack the downloaded archive (or prepare the cloned source code tree for build actions), build the source code and copy build results to a place where it is going to be used. The directory names are supposed to be: `${pkg}-${V}.dwn`, `${pkg}-${V}.src`, `${pkg}-${V}`. The `i_make_binary_symlink` subroutine makes a handy symlink for binary results: `${pkg}-${V} -> ${pkg}.bin`.

## Typical structure of a function to build some package

Besides the `i_make_binary_symlink` subroutine, there are many others that help to do some common actions. For example, if the source code is on github and is is built using standard autoconf/make process, the `dnb_foobar` function may look like:

```
function dnb_foobar() {
    local pkg="foobar"
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

In this example, we use predefined functions `du_github`, `bi_autoconf_make` and `i_make_binary_symlink`. The first letters in name of functions of this kind denote which of `d`, `u`, `b`, `i` build modes it handles. Build modes are just acronyms from `download`, `build`, `unpack`, `install`. So, for example `du_github` function does some actions for `download` and `unpack` build modes. We supply the current set of modes as a last argument (`$m`).

## List of predefined helper functions

Full list of ready-made functions can be seen in the `db.inc` file. We have some functions to get the source code from gitlab, github, svn repositories etc; besides that we have functions interfacing the popular build methods like `autoconf/make` and `cmake`. 


## Assumed version naming

As for version signature, the `du_github` and similar functions take versions as tag names or as branch names combined with changeset id, like: `foobar:some_tag`, `foobar:HEAD^master` or `foobar:0987fe4^myfeaturebranch`.

## Command line arguments handling

In the typical `dnb.sh` flow, we pass all the command line arguments to `dubi_main` subroutine. The expected arguments are:

```
./dnb.sh PACKAGE1:MODE1 PACKAGE2:MODE2 ...

or

./dnb.sh :MODE

or

./dnb.sh
```

The list of packages that are expected in set in the mandatory `PACKAGES` variable in the `./dnb.sh` script. When the unknown package name is provided, it is just ignored without any warning. The `MODE` that we put after colon is combination of `d`, `u`, `b` and `i` letters. Setting up the mode, we denote which actions for every package must be done. When no package names are given, but only `:MODE` argument is provided, we apply this mode for every package from the `PACKAGES` variable contents. When no arguments are provided, we apply a default mode to each package from the `PACKAGES`. The default mode is `dubi`, unless is overriden by external setting of optionsl `DEFAULT_BUILD_MODE` variable.

## External environment variables

A couple of external environment variables can be provided to modify the behaviour of the script. 

- `export DNB_PACKAGE_VERSIONS="PACKAGE:VERSION PACKAGE:VERSION ..."` -- override some of the package versions defined by mandatory `VERSIONS` variable from `dnb.sh`
- `export DNB_DEFAULT_BUILD_MODE=":[d][u][b][i]"` -- override the default build mode (default is `:dubi`)
- `export DNB_MAKE_PARALLEL_LEVEL=N` -- set the assumed argument for `-j` key when `make` utility is called.

## Environment file and environment settings

It is supposed that `dnb.sh` script includes the local `env.sh` file. We expect this file to have two bash functions:

- `env_init_global()` -- this function is called by `environment_check_main()` subroutine. It is supposed to load all necesary modules and set all the paths and environment variables. We also may set the variables from "External environment variables" list (for example, the `DNB_MAKE_PARALLEL_LEVEL` is good fit to set here).
- `env_init(package)` -- this function is meant to be package specific and is called by `environment_check_specific "$pkg"` subroutine. We normally call it from each specific `dnb_package` function. Here it is reasonable to set package-cpecific environment variables. It is better to name those variables using some package-related prefix.



