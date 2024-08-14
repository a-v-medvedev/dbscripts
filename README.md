# dbscripts

A set of helper scripts for source code dependencies building (forked from https://gitlab.com/xamg/dbscripts)

## Usage method

You can add this repository as a submodule to your own repository.

Then you can add a `dnb.sh` script that is looking like:

```
#!/bin/bash

DNB_DBSCRIPTSDIR=./dbscripts
DNB_YAML_CONFIG="dnb.yaml"

source $DNB_DBSCRIPTSDIR/includes.inc

function dnb_foo() {
    generic_prolog "foo" $* || return 0
    du_github "github-account" ""
    bi_make "cd src" "CXX=mpicxx SOME_OPTION=VALUE" "foo-result-binary-name"
    generic_epilog
}

function dnb_bar() {
    generic_prolog "bar" $* || return 0
    this_mode_is_set "d" && { do_actions_for_source_downloading; do_something_else_after_downloading; }
    this_mode_is_set "u" && { do_some_stuff_for_unpacking; do_something_else_after_unpacking; }
    this_mode_is_set "b" && { do_build_actions_in_unpacked_source_tree; }
    this_mode_is_set "i" && { do_install_steps; }
    generic_epilog
}

function dnb_sandbox() {
    # optional final actions
    mkdir -p sandbox
    cd sandbox
    cp -r ../foo.bin/* .
    cp -r ../bar.bin/* .
    generate_psubmit_opt "."
    cd $DNB_INSTALL_DIR
}

source "$DNB_DBSCRIPTSDIR/yaml-config.inc"
```

Here we define two special functions to download source code archive or clone the source code tree, unpack the downloaded archive (or prepare the cloned source code tree for build actions), build the source code and copy build results to a place where it is going to be used. The directory names are supposed to be: `${pkg}-${V}.dwn`, `${pkg}-${V}.src`, `${pkg}-${V}`.

The `dnb_bar` shows an example of the most generalized form for such a function. The `dnb_foo` illustrates the way some ready-made subroutines can be utilized. The `dnb_sandbox` is optional and is executed after all actions are complete, and only in case the installation action was selected in the build mode definitions. Please note, that in the context of this example and in other contexts we use acronyms `d`, `u`, `b` and `i` to address 4 main action modes of the utility: `download`, `build`, `unpack` and `install`. 

The `dnb.yaml` for this fictional case may look like:

```
---
packages:
  - [ foo ]
  - [ bar ]
versions:
  - condition: true
    list:
      - foo:0.7.0
      - bar:HEAD^0.8rc
target_dirs:
  - [ foo.bin ]
  - [ bar.bin ]
psubmit:
  target_bin: foobar
  job_name: foobar-basic-test
specific:
  - adhoc.yaml
  - machine.yaml
preamble:
  - "echo \"Download and build script for FOOBAR, for docs please refer: http://xxx.com/foobar.\""
  - "[ -f ./machine.yaml ] || fatal \"machine.yaml symlink to a architecture-specific file must exist.\""
script:
  - dubi_main "$*"
...
```

The `machine.yaml` is meant to be architecture dependant and may look like:

```
checks: [ cuda:12.0 ]
environment:
  - module load nvidia-hpc-sdk/24.3 cmake
  - export CXXFLAGS="--diag_suppress code_is_unreachable"
settings:
  parallel_level: 16
  default_mode: ":bi"
psubmit:
  nnodes: 1
  ppn: 8
  nth: 10
  ngpus: 4
  account: my_account_name
  time_limit: 10
  node_type: accel_debug
  resource_handling: qos
  generic_resources: gpu:4
  init_commands: "module load nvidia-hpc-sdk/24.3"
  mpi_script: ompi4
  batch_script: slurm
per-package:
  foo:
    - |
      export FOO_TUNING_PARAMETER=42

  bar:
    - |
      [ -f /some/path/to/file ] && export BAR_TUNING_PARAMETER=24 
...
```

The `adhoc.yaml` in this setup is optional and may contain some special settings and overrides for a specific build session.

Here we define a special function to download source code, unpack the downloaded archive (or prepare the cloned source code tree for build actions), build the source code and copy build results to a place where it is going to be used. The directory names are supposed to be: `${pkg}-${V}.dwn`, `${pkg}-${V}.src`, `${pkg}-${V}`.

## Typical structure of a function to build some package

Besides the `du_github` and `bi_make` subroutines, there are many others that help to do some common actions. For example, if the source code is on github and is is built using standard autoconf/make process, the `dnb_foobar` function may look like:

```
function dnb_foobar() {
    generic_prolog "foobar" $*
    du_gitlab "gitlab-account" "<optional-prefix>" "<optional-server-path>" "<optional-token>" "<optional-project-id>"
    local OPTS=""
    OPTS="$OPTS CPPFLAGS=-I$DNB_INSTALL_DIR/something.bin/include"
    OPTS="$OPTS LDFLAGS=-L$DNB_INSTALL_DIR/something.bin/lib"
    OPTS="$OPTS --enable-some-feature"
    bi_autoconf_make "<optional-shell-commands>" "$OPTS" 
    generic_epilog 
}
```

In this example, we use predefined functions `du_gitlab` and `bi_autoconf_make`. The first letters in name of functions of this kind denote which of `d`, `u`, `b`, `i` build modes it handles. Build modes are just acronyms from `download`, `build`, `unpack`, `install`. So, for example `du_github` function does some actions for `download` and `unpack` build modes.

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

The list of packages that are expected in set in the mandatory `DNB_PACKAGES` variable in the `./dnb.sh` script. When the unknown package name is provided, it is just ignored without any warning. The `MODE` that we put after colon is combination of `d`, `u`, `b` and `i` letters. Setting up the mode, we denote which actions for every package must be done. When no package names are given, but only `:MODE` argument is provided, we apply this mode for every package from the `DNB_PACKAGES` variable contents. When no arguments are provided, we apply a default mode to each package from the `DNB_PACKAGES`. The default mode is `dubi`, unless is overriden by external setting of optionsl `DNB_DEFAULT_BUILD_MODE` variable.

## External environment variables

A couple of external environment variables can be provided to modify the behaviour of the script. 

- `export DNB_PACKAGE_VERSIONS="PACKAGE:VERSION PACKAGE:VERSION ..."` -- override some of the package versions defined by mandatory `VERSIONS` variable from `dnb.sh`
- `export DNB_DEFAULT_BUILD_MODE=":[d][u][b][i]"` -- override the default build mode (default is `:dubi`) (better to define it as a `settings/default_mode` in a yaml file)
- `export DNB_MAKE_PARALLEL_LEVEL=N` -- set the assumed argument for `-j` key when `make` utility is called. (better to define it as a `settings/parallel_level` in a yaml file)

