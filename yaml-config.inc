#
# NOTE: this code uses `yq` utility for YAML file processing. The easiest way to get it:
#  $ wget https://github.com/mikefarah/yq/releases/latest/download/yq_linux_amd64 -O ~/bin/yq
#

function update_setting() {
  local yaml=$1
  local group=$2
  local setting=$3
  local variable_name=$4
  which yq >& /dev/null || fatal "The utility yq must be available"
  yq e "." "$yaml" >& /dev/null || fatal "The config file $yaml seems to be invalid: check it with: yq e \".\" $yaml."
  if [ "$(cat $yaml | yq ".$group")" != "null" ]; then
    if [ "$(cat $yaml | yq ".$group.$setting")" != "null" ]; then
      set +u
      local val=${!variable_name}
      set -u
      [ -z "$val" ] || { echo "WARNING: overriding $group/$setting setting is ignored in $yaml: preserved value $variable_name=$val"; return; }
      export $variable_name="$(cat $yaml | yq ".$group.$setting")"
    fi
  fi
  return 0
}

function parse_cfg_yaml() {
  local yaml=$1
  which yq >& /dev/null || fatal "The utility yq must be available"
  yq e "." "$yaml" >& /dev/null || fatal "The config file $yaml seems to be invalid: check it with: yq e \".\" $yaml."
  if [ "$(cat $yaml | yq '.checks')" != "null" ]; then
  for check in $(cat $yaml | yq '.checks | join(" ")'); do 
    case $check in
      cuda*) export DNB_CHECK_CUDA=$(echo $check | cut -d: -f2 -s);;
      cmake) export DNB_CHECK_CMAKE=$(echo $check | cut -d: -f2 -s);;
      ccomp) export DNB_CHECK_CCOMP="";;
      cxxcomp) export DNB_CHECK_CXXCOMP="";;
    esac
  done
  fi

  npkgs=$(cat $yaml | yq '.packages | length')
  if [ "$npkgs" != 0 ]; then
  local pkgs=""
  for i in $(seq 0 1 $(expr $npkgs - 1)); do 
    local condition=$(cat $yaml | yq ".packages[$i][1]")
    local pkg=$(cat $yaml | yq ".packages[$i][0]")
    [ "$pkg" == "null" ] && continue
    if [ "$condition" != "null" ]; then
      eval "$condition" || continue
    fi
    [ -z "$pkgs" ] && export pkgs=$pkg && continue
    export pkgs="$pkgs $pkg"
  done
  set +u
  export DNB_PACKAGES="$pkgs $DNB_PACKAGES"
  set -u
  fi

  ndirs=$(cat $yaml | yq '.target_dirs | length')
  if [ "$ndirs" != 0 ]; then
  for i in $(seq 0 1 $(expr $ndirs - 1)); do 
    local condition=$(cat $yaml | yq ".target_dirs[$i][1]")
    local dir=$(cat $yaml | yq ".target_dirs[$i][0]")
    [ "$dir" == "null" ] && continue
    if [ "$condition" != "null" ]; then
      eval "$condition" || continue
    fi
    set +u
    [ -z "$DNB_TARGET_DIRS" ] && export DNB_TARGET_DIRS=$dir && continue
    set -u
    export DNB_TARGET_DIRS="$DNB_TARGET_DIRS $dir"
  done
  fi

  nvers=$(cat $yaml | yq '.versions | length')
  if [ "$nvers" != 0 ]; then
  for i in $(seq 0 1 $(expr $nvers - 1)); do 
    local condition=$(cat $yaml | yq ".versions[$i].condition")
    local versions_array=$(cat $yaml | yq ".versions[$i].list | join(\" \")")
    [ "$versions_array" == "null" ] && continue
    [ "$versions_array" == "" ] && continue
    if [ "$condition" != "null" ]; then
      eval "$condition" || continue
    fi
    for v in $versions_array; do
      pkg=$(echo $v | cut -d: -f1) 
      vnum=$(echo $v | cut -d: -f2 -s)
      [ -z "$vnum" ] && fatal "Error in version definition for pkg: $pkg"
      set +u
      [ -z "$DNB_VERSIONS" ] && export DNB_VERSIONS="$v" && continue
      set -u
      ##echo $DNB_VERSIONS | egrep -q "(^$pkg:)|( $pkg:)" && continue 
      echo $DNB_VERSIONS | egrep -q "(^$pkg:)|( $pkg:)" && fatal "duplicate definition of version for pkg: $pkg"
      export DNB_VERSIONS="$DNB_VERSIONS $v"
    done
  done
  fi

  nlines=$(cat $yaml | yq '.environment | length')
  if [ "$nlines" != 0 ]; then
    for i in $(seq 0 1 $(expr $nlines - 1)); do 
      local line=$(cat $yaml | yq ".environment[$i]")
      [ "$line" == "null" ] && continue
      echo $line
      eval $line
    done
  fi

  update_setting $yaml settings parallel_level DNB_MAKE_PARALLEL_LEVEL
  update_setting $yaml settings default_mode DNB_DEFAULT_BUILD_MODE

  update_setting $yaml psubmit nnodes PSUBMIT_OPT_NNODES
  update_setting $yaml psubmit ppn PSUBMIT_OPT_PPN
  update_setting $yaml psubmit nth PSUBMIT_OPT_NTH
  update_setting $yaml psubmit ngpus PSUBMIT_OPT_NGPUS
  update_setting $yaml psubmit queue_name PSUBMIT_OPT_QUEUE

  update_setting $yaml psubmit queue_suffix PSUBMIT_OPT_QUEUE_SUFFIX
  update_setting $yaml psubmit constraint PSUBMIT_OPT_CONSTRAINT
  update_setting $yaml psubmit whitelist PSUBMIT_OPT_WHITELIST
  update_setting $yaml psubmit blacklist PSUBMIT_OPT_BLACKLIST
  update_setting $yaml psubmit comment PSUBMIT_OPT_COMMENT
  update_setting $yaml psubmit before PSUBMIT_OPT_BEFORE
  update_setting $yaml psubmit after PSUBMIT_OPT_AFTER
  update_setting $yaml psubmit target_bin PSUBMIT_OPT_TARGET_BIN
  update_setting $yaml psubmit job_name PSUBMIT_OPT_JOB_NAME

  update_setting $yaml psubmit account PSUBMIT_OPT_ACCOUNT
  update_setting $yaml psubmit time_limit PSUBMIT_OPT_TIME_LIMIT
  update_setting $yaml psubmit node_type PSUBMIT_OPT_NODETYPE
  update_setting $yaml psubmit resource_handling PSUBMIT_OPT_RESOURCE_HANDLING
  update_setting $yaml psubmit generic_resources PSUBMIT_OPT_GENERIC_RESOURCES

  update_setting $yaml psubmit init_commands PSUBMIT_OPT_INIT_COMMANDS
  update_setting $yaml psubmit injob_init_commands PSUBMIT_OPT_INJOB_INIT_COMMANDS
  update_setting $yaml psubmit mpi_script PSUBMIT_OPT_MPIEXEC
  update_setting $yaml psubmit batch_script PSUBMIT_OPT_BATCH

  if [ "$(cat $yaml | yq '.per-package')" != "null" ]; then
    local pkgs=$(cat $yaml | yq '.per-package | keys | .[]')
    if [ "$pkgs" != "null" ]; then
      for pkg in $pkgs; do
        local v=DNB_PACKAGE_INIT_SCRIPT__$(echo "$pkg" | sed 's/-/_/g')
        [ -v $v ] && echo "WARNING: not overriding the per-package script for package: $pkg" && continue
        local script_file_name=$PWD/$(mktemp .dnb.script.XXXXXXXX)
        cat $yaml | yq ".per-package.$pkg[0]" > "$script_file_name"
        [ "$(cat $script_file_name)" != "" -a "$(cat $script_file_name)" != "null" ] && eval $v=$script_file_name
        #if [ -v $v ]; then
        #  echo ">>>>  >>>> $v:"
        #  cat "${!v}" 
        #  echo ">>>>>>>>>>"
        #  export $v
        #fi
      done
    fi
  fi
  return 0
}


function exec_preamble_yaml() {
  local yaml=$1
  which yq >& /dev/null || fatal "The utility yq must be available"
  yq e "." "$yaml" >& /dev/null || fatal "The config file $yaml seems to be invalid: check it with: yq e \".\" $yaml."
  nlines=$(cat $yaml | yq '.preamble | length')
  [ "$nlines" == 0 ] && return
  for i in $(seq 0 1 $(expr $nlines - 1)); do 
    local line=$(cat $yaml | yq ".preamble[$i]")
    [ "$line" == "null" ] && continue
    #echo $line
    eval $line
  done
  return 0
}

function parse_script_yaml() {
  local yaml=$1
  shift
  which yq >& /dev/null || fatal "The utility yq must be available"
  yq e "." "$yaml" >& /dev/null || fatal "The config file $yaml seems to be invalid: check it with: yq e \".\" $yaml."
  nlines=$(cat $yaml | yq '.script | length')
  [ "$nlines" == 0 ] && return
  export DNB_SCRIPT_LINES=""
  for i in $(seq 0 1 $(expr $nlines - 1)); do 
    local script_file_name=$PWD/$(mktemp .dnb.script.XXXXXXXX)
    cat $yaml | yq ".script[$i]" > "$script_file_name"
    [ "$(cat $script_file_name)" != "" -a "$(cat $script_file_name)" != "null" ] && DNB_SCRIPT_LINES="$DNB_SCRIPT_LINES $script_file_name"
  done
  return 0
}

function exec_script() {
  [ -v DNB_SCRIPT_LINES ] || return 0	
  [ -z "$DNB_SCRIPT_LINES" ] && return 0	
  for file in $DNB_SCRIPT_LINES; do
    [ -f "$file" ] || continue
    echo ">> dbscripts: EXECUTE:" $(cat $file)
    source $file $*
    rm -f $file
  done
  return 0
}

function parse_specific_yamls() {
  local yaml=$1
  which yq >& /dev/null || fatal "The utility yq must be available"
  yq e "." "$yaml" >& /dev/null || fatal "The config file $yaml seems to be invalid: check it with: yq e \".\" $yaml."
  for spec in $(cat $yaml | yq '.specific | join(" ")'); do
    [ -f "$spec" ] && echo ">> dbscripts: Processing $spec:" && parse_cfg_yaml "$spec"
  done
  return 0
}

function remove_temps() {
  cd $DNB_INSTALL_DIR
  rm -f .dnb.script.*
  cd $DNB_ORIGINAL_PWD
  rm -f .dnb.script.*
  return 0
}

[ -z "$DNB_DBSCRIPTSDIR" ] && DNB_DBSCRIPTSDIR=dbscripts
[ -z "$DNB_YAML_CONFIG" ] && DNB_YAML_CONFIG=dnb.yaml

DNB_ORIGINAL_PWD=$PWD
trap remove_temps EXIT

export DNB_CHECK_CCOMP=""
export DNB_CHECK_CMAKE=""
export DNB_CHECK_CUDA=""
export DNB_CHECK_CXXCOMP=""
export DNB_DEFAULT_BUILD_MODE=""
export DNB_MAKE_PARALLEL_LEVEL=""
export DNB_PACKAGES=""
export DNB_SCRIPT_LINES=""
export DNB_TARGET_DIRS=""
export DNB_VERSIONS=""

export PSUBMIT_OPT_NNODES=""
export PSUBMIT_OPT_PPN=""
export PSUBMIT_OPT_NTH=""
export PSUBMIT_OPT_NGPUS=""
export PSUBMIT_OPT_QUEUE=""

export PSUBMIT_OPT_QUEUE_SUFFIX=""
export PSUBMIT_OPT_CONSTRAINT=""
export PSUBMIT_OPT_WHITELIST=""
export PSUBMIT_OPT_BLACKLIST=""
export PSUBMIT_OPT_COMMENT=""
export PSUBMIT_OPT_BEFORE=""
export PSUBMIT_OPT_AFTER=""
export PSUBMIT_OPT_TARGET_BIN=""
export PSUBMIT_OPT_JOB_NAME=""

export PSUBMIT_OPT_ACCOUNT=""
export PSUBMIT_OPT_TIME_LIMIT=""
export PSUBMIT_OPT_NODETYPE=""
export PSUBMIT_OPT_RESOURCE_HANDLING=""
export PSUBMIT_OPT_GENERIC_RESOURCES=""

export PSUBMIT_OPT_INIT_COMMANDS=""
export PSUBMIT_OPT_INJOB_INIT_COMMANDS=""
export PSUBMIT_OPT_MPIEXEC=""
export PSUBMIT_OPT_BATCH=""

[ -f "$DNB_YAML_CONFIG" ] || fatal "can't open main config: $DNB_YAML_CONFIG"
exec_preamble_yaml "$DNB_YAML_CONFIG"

started=$(date "+%s")
echo "Download and build started at timestamp: $started."
echo "----------"

parse_specific_yamls "$DNB_YAML_CONFIG"
parse_cfg_yaml "$DNB_YAML_CONFIG"
parse_script_yaml "$DNB_YAML_CONFIG" $*

[ -v DNB_INSTALL_DIR ] || DNB_INSTALL_DIR=""
[ -z "$DNB_INSTALL_DIR" ] || cd $DNB_INSTALL_DIR   
environment_check_main || fatal "Environment is not supported, exiting"

echo ">> dbscripts: Package list: $DNB_PACKAGES"

exec_script $*

finished=$(date "+%s")
echo "----------"
echo "Full operation time: $(expr $finished - $started) seconds."

