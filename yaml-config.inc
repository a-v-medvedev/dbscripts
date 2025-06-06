#
# NOTE: this code uses `yq` utility for YAML file processing. The easiest way to get it:
#
#  $ wget https://github.com/mikefarah/yq/releases/latest/download/yq_linux_amd64 -O ~/bin/yq
#
# You may set DNB_YQ as exact path to right yq binary
#


function yq_check() {
  local yaml=$1
  which ${DNB_YQ} >& /dev/null || fatal "The utility yq must be available (tried to exec: ${DNB_YQ})."
  echo -ne "---\naaa: [ bbb,ccc ]\n..." | ${DNB_YQ} e "." 2>/dev/null | grep -q "bbb, ccc" || fatal "yq utility is disfunctional (tried to exec: ${DNB_YQ}). Wrong version?"
  ${DNB_YQ} e "." "$yaml" >& /dev/null || fatal "The config file $yaml seems to be invalid: check it with: yq e \".\" $yaml."
}

function update_setting() {
  local yaml=$1
  local group=$2
  local setting=$3
  local variable_name=$4
  yq_check "$yaml"
  if [ "$(cat $yaml | ${DNB_YQ} ".$group")" != "null" ]; then
    if [ "$(cat $yaml | ${DNB_YQ} ".$group.$setting")" != "null" ]; then
      set +u
      local val=${!variable_name}
      set -u
      [ -z "$val" ] || { echo ">> dbscripts: WARNING: overriding $group/$setting setting is ignored in $yaml: preserved value $variable_name=$val"; return; }
      export $variable_name="$(cat $yaml | ${DNB_YQ} ".$group.$setting")"
    fi
  fi
  return 0
}

function parse_cfg_yaml() {
  local yaml=$1
  yq_check "$yaml"
  if [ "$(cat $yaml | ${DNB_YQ} '.checks')" != "null" -a ! -v DNB_OMIT_CHECKS ]; then
  for check in $(cat $yaml | ${DNB_YQ} '.checks | join(" ")'); do 
    case $check in
      cuda*) export DNB_CHECK_CUDA=$(echo $check | cut -d: -f2 -s);;
      cmake) export DNB_CHECK_CMAKE=$(echo $check | cut -d: -f2 -s);;
      ccomp) export DNB_CHECK_CCOMP="";;
      cxxcomp) export DNB_CHECK_CXXCOMP="";;
    esac
  done
  fi

  npkgs=$(cat $yaml | ${DNB_YQ} '.packages | length')
  if [ "$npkgs" != 0 ]; then
  local pkgs=""
  for i in $(seq 0 1 $(expr $npkgs - 1)); do 
    local condition=$(cat $yaml | ${DNB_YQ} ".packages[$i][1]")
    local pkg=$(cat $yaml | ${DNB_YQ} ".packages[$i][0]")
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

  ndirs=$(cat $yaml | ${DNB_YQ} '.target_dirs | length')
  if [ "$ndirs" != 0 ]; then
  for i in $(seq 0 1 $(expr $ndirs - 1)); do 
    local condition=$(cat $yaml | ${DNB_YQ} ".target_dirs[$i][1]")
    local dir=$(cat $yaml | ${DNB_YQ} ".target_dirs[$i][0]")
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

  nlines=$(cat $yaml | ${DNB_YQ} '.environment | length')
  if [ "$nlines" != 0 ]; then
    for i in $(seq 0 1 $(expr $nlines - 1)); do 
      local line=$(cat $yaml | ${DNB_YQ} ".environment[$i]")
      [ "$line" == "null" ] && continue
      echo $line
      eval $line
    done
  fi

  nvers=$(cat $yaml | ${DNB_YQ} '.versions | length')
  if [ "$nvers" != 0 ]; then
  for i in $(seq 0 1 $(expr $nvers - 1)); do 
    local condition=$(cat $yaml | ${DNB_YQ} ".versions[$i].condition")
    local versions_array=$(cat $yaml | ${DNB_YQ} ".versions[$i].list | join(\" \")")
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
  update_setting $yaml psubmit reservation PSUBMIT_OPT_RESERVATION
  update_setting $yaml psubmit time_limit PSUBMIT_OPT_TIME_LIMIT
  update_setting $yaml psubmit node_type PSUBMIT_OPT_NODETYPE
  update_setting $yaml psubmit resource_handling PSUBMIT_OPT_RESOURCE_HANDLING
  update_setting $yaml psubmit generic_resources PSUBMIT_OPT_GENERIC_RESOURCES

  update_setting $yaml psubmit init_commands PSUBMIT_OPT_INIT_COMMANDS
  update_setting $yaml psubmit injob_init_commands PSUBMIT_OPT_INJOB_INIT_COMMANDS
  update_setting $yaml psubmit mpi_script PSUBMIT_OPT_MPIEXEC
  update_setting $yaml psubmit batch_script PSUBMIT_OPT_BATCH

  if [ "$(cat $yaml | ${DNB_YQ} '.per-package')" != "null" ]; then
    local pkgs=$(cat $yaml | ${DNB_YQ} '.per-package | keys | .[]')
    if [ "$pkgs" != "null" ]; then
      for pkg in $pkgs; do
        local v=DNB_PACKAGE_INIT_SCRIPT__$(echo "$pkg" | sed 's/-/_/g')
        [ -v $v ] && echo ">> dbscripts: WARNING: not overriding the per-package script for package: $pkg" && continue
        local script_file_name=$PWD/$(mktemp .dnb.script.XXXXXXXX)
        cat $yaml | ${DNB_YQ} ".per-package.$pkg[0]" > "$script_file_name"
        [ "$(cat $script_file_name)" != "" -a "$(cat $script_file_name)" != "null" ] && eval $v=$script_file_name
      done
    fi
  fi
  return 0
}


function exec_preamble_yaml() {
  local yaml=$1
  yq_check "$yaml"
  nlines=$(cat $yaml | ${DNB_YQ} '.preamble | length')
  [ "$nlines" == 0 ] && return
  for i in $(seq 0 1 $(expr $nlines - 1)); do 
    local line=$(cat $yaml | ${DNB_YQ} ".preamble[$i]")
    [ "$line" == "null" ] && continue
    #echo $line
    eval $line
  done
  return 0
}

function parse_script_yaml() {
  local yaml=$1
  shift
  yq_check "$yaml"
  nlines=$(cat $yaml | ${DNB_YQ} '.script | length')
  [ "$nlines" == 0 ] && return
  export DNB_SCRIPT_LINES=""
  for i in $(seq 0 1 $(expr $nlines - 1)); do 
    local script_file_name=$PWD/$(mktemp .dnb.script.XXXXXXXX)
    cat $yaml | ${DNB_YQ} ".script[$i]" > "$script_file_name"
    [ "$(cat $script_file_name)" != "" -a "$(cat $script_file_name)" != "null" ] && DNB_SCRIPT_LINES="$DNB_SCRIPT_LINES $script_file_name"
  done
  return 0
}

function exec_script() {
  [ -v DNB_SCRIPT_LINES ] || return 0	
  [ -z "$DNB_SCRIPT_LINES" ] && return 0	
  for file in $DNB_SCRIPT_LINES; do
    [ -f "$file" ] || continue
    if [ $(head -c1 "$file") == "@" ]; then
      sed -i 's/^@//' "$file"
    else
      echo ">> dbscripts: EXECUTING:" $(cat $file)
    fi
    source $file $* || true
    rm -f $file
  done
  return 0
}

function parse_specific_yamls() {
  local yaml=$1
  yq_check "$yaml"
  for spec in $(cat $yaml | ${DNB_YQ} '.specific | join(" ")'); do
    [ -f "$spec" ] && echo ">> dbscripts: Processing $spec:" && parse_cfg_yaml "$spec"
  done
  return 0
}

function remove_temps() {
  if [ -v DNB_INSTALL_DIR ]; then
    cd $DNB_INSTALL_DIR
    rm -f .dnb.script.*
  fi
  cd $DNB_ORIGINAL_PWD
  rm -f .dnb.script.*
  return 0
}

[ -z "$DNB_DBSCRIPTSDIR" ] && DNB_DBSCRIPTSDIR=dbscripts
[ -z "$DNB_YAML_CONFIG" ] && DNB_YAML_CONFIG=dnb.yaml
DNB_ORIGINAL_PWD=$PWD

if [ -v DNB_YQ_DOWNLOAD_URL ]; then
    [ -v DNB_YQ ] && fatal "DNB_YQ_DOWNLOAD_URL and DNB_YQ are set at the same time. Choose one option."
    export DNB_YQ=$DNB_ORIGINAL_PWD/$DNB_DBSCRIPTSDIR/yq
    if [ ! -f "$DNB_YQ" ]; then
        wget "$DNB_YQ_DOWNLOAD_URL" -O $DNB_DBSCRIPTSDIR/yq || fatal "Failed to download yq utility, url: $DNB_YQ_DOWNLOAD_URL"
        chmod +x "$DNB_YQ"
    fi
fi

DNB_YQ=${DNB_YQ:-"yq"}

trap remove_temps EXIT

unset DNB_CHECK_CCOMP
unset DNB_CHECK_CMAKE
unset DNB_CHECK_CUDA
unset DNB_CHECK_CXXCOMP
unset DNB_DEFAULT_BUILD_MODE
unset DNB_MAKE_PARALLEL_LEVEL
unset DNB_PACKAGES
unset DNB_SCRIPT_LINES
unset DNB_TARGET_DIRS
unset DNB_VERSIONS

unset PSUBMIT_OPT_NNODES
unset PSUBMIT_OPT_PPN
unset PSUBMIT_OPT_NTH
unset PSUBMIT_OPT_NGPUS
unset PSUBMIT_OPT_QUEUE

unset PSUBMIT_OPT_QUEUE_SUFFIX
unset PSUBMIT_OPT_CONSTRAINT
unset PSUBMIT_OPT_WHITELIST
unset PSUBMIT_OPT_BLACKLIST
unset PSUBMIT_OPT_COMMENT
unset PSUBMIT_OPT_BEFORE
unset PSUBMIT_OPT_AFTER
unset PSUBMIT_OPT_TARGET_BIN
unset PSUBMIT_OPT_JOB_NAME

unset PSUBMIT_OPT_ACCOUNT
unset PSUBMIT_OPT_RESERVATION
unset PSUBMIT_OPT_TIME_LIMIT
unset PSUBMIT_OPT_NODETYPE
unset PSUBMIT_OPT_RESOURCE_HANDLING
unset PSUBMIT_OPT_GENERIC_RESOURCES

unset PSUBMIT_OPT_INIT_COMMANDS
unset PSUBMIT_OPT_INJOB_INIT_COMMANDS
unset PSUBMIT_OPT_MPIEXEC
unset PSUBMIT_OPT_BATCH

[ -f "$DNB_YAML_CONFIG" ] || fatal "can't open main config: $DNB_YAML_CONFIG"
exec_preamble_yaml "$DNB_YAML_CONFIG"

started=$(date "+%s")
echo "Started at timestamp: $started."
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

