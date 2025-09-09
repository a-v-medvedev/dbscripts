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

function here-document() {
    local file="$1"
    local dir=$(dirname $file)
    if [ "$dir" != "." -a "$dir" != ".." ]; then 
        [ -e "$dir" ] || mkdir -p "$dir"
    fi
    cat | sed 's/^[ \t]*//' > $file
}

function mkdatalink() {
    local dir="$1"
    local file="$2"
    local oldpwd=$PWD
    [ -e "$HOME/data/$file" ] || return 0
    cd "$dir"
    [ -L "$file" ] && rm -rf "$file"
    [ -e "$file" ] || ln -s "$HOME/data/$file" .
    cd "$oldpwd"
}

function set_path_variable_if_missing() {
    local prefix=$1    #NEMO
    local var=$2       #NETCDF_C
    local default_value=$3
    local full_var_name=${prefix}_${var}_PATH   #NEMO_NETCDF_C_PATH
    local use_prebuild_name=${prefix}_USE_PREBUILT_${var}
    if [ -z "${!use_prebuild_name}" -o "${!use_prebuild_name}" == FALSE ]; then
        [ -z "${!full_var_name}" ] || fatal "$full_var_name must not be set since $use_prebuild_name is set to FALSE";
        export ${full_var_name}=${3};
    else
        [ -z "${!full_var_name}" ] && fatal "direct path to binaries is required: $full_var_name is not set";
    fi
}

function dvc_checkout() {
    local dir=$1  
    local branch=$2 
    [ -e "$dir/scripts" -a -e "$dir/data" ] && return
    mkdir -p $dir/scripts
    cd $dir/scripts
    eval $NEMO_DVC_INIT
    git clone -b $branch https://earth.bsc.es/gitlab/kkeller/dvc-cache-de340.git
    cd dvc-cache-de340
    $NEMO_DVC_CMD config --project cache.dir $NEMO_DVC_CACHE_DIR
    $NEMO_DVC_CMD checkout
    cd scripts/
    chmod +x ./create_experiment.sh
    EXPID=00 ./create_experiment.sh
    cd ../../..
    ln -s scripts/dvc-cache-de340/scripts/exp-00 data
    sed -i 's/nn_stock =.*/nn_stock = -1/' data/namelist_cfg
    sed -i 's/nn_stocklist =.*/nn_stocklist = -9999/' data/namelist_cfg
    sed -i 's/ln_timing =.*$/&\nln_timing_detail = .true.\nsn_cfctl%l_runstat = .true./' data/namelist_cfg
    sed -i 's!\(<variable id="info_level".*>\).*</variable>!\1-2</variable>!' data/iodef.xml
    cd ..
}

function dnb_psyclone() {
    generic_prolog "psyclone" $* || return 0
    local psyclone_script="share/psyclone/examples/nemo/scripts/insert_loop_parallelism.py"
    du_github "stfc" "" 
    if this_mode_is_set 'u'; then
        cd "${pkg}-${V}.src"
        [ -d psyclone_venv ] && rm -rf psyclone_venv
        python -m venv psyclone_venv
        source psyclone_venv/bin/activate 
        pip install .
        patch_name="../patches_5.0/__patch_nemo-5.0_insert_loop_parallelism.diff"
        file_to_patch="psyclone_venv/$psyclone_script"
        patch_single_file "$file_to_patch" "$patch_name"
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'b'; then
        cd "${pkg}-${V}.src"
        [ -d psyclone_venv ] || fatal "python virtual environment for psyclone not found."
        source psyclone_venv/bin/activate
        export PSYCLONE_HOME=$DNB_INSTALL_DIR/"${pkg}-${V}.src"/psyclone_venv
        [ -e "psyclone_venv/$psyclone_script" ] && { export NEMO_NEW_PSYCLONE_INTEGRATION=TRUE; export PSYCLONE__SCRIPT="$PSYCLONE_HOME/$psyclone_script"; }
        cd $DNB_INSTALL_DIR      
    fi
    this_mode_is_set 'i' && mkdir -p "${pkg}-${V}"
    generic_epilog
}

function dnb_loki() {
    generic_prolog "loki" $* || return 0
    du_github "ecmwf-ifs" ""
    if this_mode_is_set 'u'; then
        cd "${pkg}-${V}.src"
        [ -d loki_venv ] && rm -rf loki_venv
        python -m venv loki_venv
	./install --use-venv=loki_venv
	source loki-activate
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'b'; then
        cd "${pkg}-${V}.src"
        [ -d loki_venv ] || fatal "python virtual environment for loki not found."
	source loki-activate
        cd $DNB_INSTALL_DIR
    fi
    this_mode_is_set 'i' && mkdir -p "${pkg}-${V}"
    generic_epilog
}

function dnb_xios() {
    generic_prolog "xios" $* || return 0
    local dir="branchs/${pkg}-${V}"
    [ "$V" == "trunk" ] && dir="trunk"
    du_svn "https://forge.ipsl.jussieu.fr/ioserver/svn/XIOS/$dir"
    if this_mode_is_set 'b'; then
        set +u
        set_path_variable_if_missing NEMO HDF5 "$DNB_INSTALL_DIR/hdf5.bin"
        set_path_variable_if_missing NEMO NETCDF_C "$DNB_INSTALL_DIR/netcdf-c.bin"
        set_path_variable_if_missing NEMO NETCDF_FORTRAN "$DNB_INSTALL_DIR/netcdf-fortran.bin"
        fail_if_variable_not_defined XIOS_CCOMPILER XIOS_FCOMPILER XIOS_LINKER XIOS_CFLAGS XIOS_CPP XIOS_FPP
        set -u

        cd ${pkg}-${V}.src
        here-document arch/arch-dnb.env <<________EOF
            #env
            export HDF5_DIR=$NEMO_HDF5_PATH
            export NETCDF_DIR=$NEMO_NETCDF_C_PATH
            export PNETCDF_DIR=$NEMO_NETCDF_C_PATH
________EOF

        here-document arch/arch-dnb.fcm <<________EOF
            #fcm
            %CCOMPILER      $XIOS_CCOMPILER
            %FCOMPILER      $XIOS_FCOMPILER
            %LINKER         $XIOS_LINKER

            %BASE_CFLAGS    
            %PROD_CFLAGS    $XIOS_CFLAGS
            %DEV_CFLAGS     -g -traceback
            %DEBUG_CFLAGS   -DBZ_DEBUG -g -traceback -fno-inline

            %BASE_FFLAGS    -D__NONE__
            %PROD_FFLAGS    -O3 
            %DEV_FFLAGS     -g -O2 -traceback
            %DEBUG_FFLAGS   -g -traceback

            %BASE_INC       -D__NONE__
            %BASE_LD        -lstdc++

            %CPP            $XIOS_CPP
            %FPP            $XIOS_FPP
            %MAKE           gmake
________EOF

        here-document arch/arch-dnb.path <<________EOF
            #path
            NETCDF_INCDIR="-I${NEMO_NETCDF_C_PATH}/include -I${NEMO_NETCDF_FORTRAN_PATH}/include"
            NETCDF_LIBDIR="-L${NEMO_NETCDF_FORTRAN_PATH}/lib -L${NEMO_NETCDF_C_PATH}/lib"
            NETCDF_LIB="-L${NEMO_NETCDF_FORTRAN_PATH}/lib -lnetcdff -L${NEMO_NETCDF_C_PATH}/lib -lnetcdf"

            HDF5_INCDIR="-I${NEMO_HDF5_PATH}/include"
            HDF5_LIBDIR="-L${NEMO_HDF5_PATH}/lib"
            HDF5_LIB="-L${NEMO_HDF5_PATH}/lib -lhdf5_hl -L${NEMO_HDF5_PATH}/lib -lhdf5"

            OASIS_INCDIR=""
            OASIS_LIBDIR=""
            OASIS_LIB=""
________EOF
        local netcdf_option="netcdf4_seq"
        is_set_to_true XIOS_PARALLEL && netcdf_option="netcdf4_par"
        ./make_xios --netcdf_lib $netcdf_option --prod --arch dnb --job $XIOS_MAKE_PARALLEL_LEVEL || fatal "make_xios failed."
        cd $DNB_INSTALL_DIR
    fi
    if this_mode_is_set 'i'; then
        [ -d "$pkg-$V" ] && rm -rf "$pkg-$V"
        [ -e "$pkg-$V" ] && rm -f "$pkg-$V"
        mkdir -p "$pkg-$V"
        mkdir -p "$pkg-$V/lib"
        mkdir -p "$pkg-$V/inc"
        cp ${pkg}-${V}.src/bin/* ${pkg}-${V}/lib
        cp ${pkg}-${V}.src/lib/* ${pkg}-${V}/lib
        cp ${pkg}-${V}.src/inc/* ${pkg}-${V}/inc
    fi
    generic_epilog
}

function dnb_nemo() {
    generic_prolog "nemo" $* || return 0
    fail_if_variable_not_defined NEMO_CODEBASE
    NEMO_WORKLOAD_ARCHIVE=${NEMO_WORKLOAD_ARCHIVE:-}
    [ -z "$NEMO_WORKLOAD_ARCHIVE" ] || fail_if_variable_not_defined NEMO_WORKLOAD_MD5 
    [ -z "$NEMO_WORKLOAD_ARCHIVE" ] || mkdatalink "$DNB_INSTALL_DIR" "$NEMO_WORKLOAD_ARCHIVE"

    if beginswith "$NEMO_CODEBASE" "COMMUNITY"; then
        # Use open source repositories:    
        if beginswith "$V" "4.0"; then
            du_svn "https://forge.ipsl.jussieu.fr/nemo/svn/NEMO/releases/r4.0/r$V"
        elif beginswith "$V" "4.2"; then
            true  
            # TODO: add something working for 4.2 from github
            # go with github:
            # du_github ...
        elif beginswith "$V" "5.0"; then
            # NEMO_SRC_URL: forge.nemo-ocean.eu/nemo
            du_gitlab "" "nemo" "$NEMO_SRC_URL"
        fi
    fi

    if any_mode_is_set 'du'; then
        # debug workload: for DestinE we use the manually copied tar archive
        cd "${pkg}.dwn"
        if [ ! -z "$NEMO_WORKLOAD_ARCHIVE" ]; then
            # Debug workload download procedure.
            # Avoid downloading if there are pre-downloaded files:
            if [ -f "$NEMO_WORKLOAD_ARCHIVE" ]; then
                echo "$NEMO_WORKLOAD_MD5 $NEMO_WORKLOAD_ARCHIVE" | md5sum -c --status || rm -f "$NEMO_WORKLOAD_ARCHIVE"
            fi
            # No pre-downloaded archive, have to download it:
            if [ ! -f "$NEMO_WORKLOAD_ARCHIVE" ]; then
                if [ -f "../$NEMO_WORKLOAD_ARCHIVE" ]; then  
                    rm -f "$NEMO_WORKLOAD_ARCHIVE"
                    ln -s "../$NEMO_WORKLOAD_ARCHIVE" .
                else
                    # We really have to download:
                    if beginswith "$V" "4.0"; then 
                        # download from https://zenodo.org/record/2640723#.ZGOnotJByV4  
                        wget -nv -O "$NEMO_WORKLOAD_ARCHIVE" "https://zenodo.org/record/2640723/files/$NEMO_WORKLOAD_ARCHIVE?download=1"
                    elif beginswith "$V" "4.2"; then
                        true
                        fatal "unsupported NEMO version for debug workload downloads."
                        # TODO add workload download code for 4.2
                    elif beginswith "$V" "5.0"; then
                        wget -nv -O "$NEMO_WORKLOAD_ARCHIVE" "$NEMO_WORKLOAD_URL/$NEMO_WORKLOAD_ARCHIVE"
                    else
                        fatal "For this NEMO version, we don't know how to get the debug workload."
                    fi 
                fi
            fi
        fi 

        cd $DNB_INSTALL_DIR
        if this_mode_is_set 'u'; then
            if is_set_to_true NEMO_USE_MAKE; then
                if ! beginswith "$NEMO_CODEBASE" "COMMUNITY"; then
                    cd ${pkg}-${V}.src
                    local makefiles_dir=makefiles
                    [ -d makefiles ] || makefiles_dir="../makefile-hack"
                    patch < $makefiles_dir/patch_makenemo.patch
                    ln -s $makefiles_dir/Makefile .
                    ln -s $makefiles_dir/finddeps.sh .
                    for f in roctx.F90 nvtx.F90 hipinit.F90 ranges.F90 cudamem.F90 rocmem.F90 memutils.F90; do
                        [ -e ./src/OCE/$f ] || touch ./src/OCE/$f
                    done
                    for f in lbclnk_gpu_sp.F90 lbclnk_gpu_dp.F90 lbclnk_gpu_nfd.F90 lbcnfd_gpu_sp.F90 lbcnfd_gpu_dp.F90 lbcnfd_nogather_gpu_sp.F90 lbcnfd_nogather_gpu_dp.F90; do
                        [ -e ./src/OCE/LBC/$f ] || touch ./src/OCE/LBC/$f
                    done
                fi
            fi
        fi
        cd $DNB_INSTALL_DIR
    fi
    if ! beginswith "$NEMO_CODEBASE" "COMMUNITY"; then
        u_apply_patches "patches"
    elif beginswith "$NEMO_CODEBASE" "COMMUNITY_4.0"; then
        u_apply_patches "patches_4.0"        
    elif beginswith "$NEMO_CODEBASE" "COMMUNITY_5.0"; then
        u_apply_patches "patches_5.0"        
    fi
    if this_mode_is_set 'b'; then
        set +u
        fail_if_variable_not_defined NEMO_CFG NEMO_SUBCOMPONENTS NEMO_KEYS_TO_DELETE NEMO_KEYS_TO_ADD
        set_path_variable_if_missing NEMO HDF5 "$DNB_INSTALL_DIR/hdf5.bin"
        set_path_variable_if_missing NEMO NETCDF_C "$DNB_INSTALL_DIR/netcdf-c.bin"
        set_path_variable_if_missing NEMO NETCDF_FORTRAN "$DNB_INSTALL_DIR/netcdf-fortran.bin"
        set_path_variable_if_missing NEMO XIOS "$DNB_INSTALL_DIR/xios.bin"
        fail_if_variable_not_defined NEMO_CPP NEMO_CC NEMO_FC NEMO_FCFLAGS NEMO_LDFLAGS NEMO_FPPFLAGS
        set -u

        cd ${pkg}-${V}.src
        [ -v NEMO_MAKE_PARALLEL_LEVEL ] || NEMO_MAKE_PARALLEL_LEVEL=$DNB_MAKE_PARALLEL_LEVEL
        psyclone_makenemo_opt=""
        if [ "$NEMO_CODEBASE" == "COMMUNITY_5.0" ]; then
            if is_set_to_true NEMO_WITH_GPU; then
                if ! is_set_to_true NEMO_WITH_FAKE_GPU; then
                    if ! is_set_to_true NEMO_OMIT_PSYCLONE; then
                        is_set_to_true NEMO_WITH_MANAGED_MEMORY || fatal "for PSyclone integration, NEMO_WITH_MANAGED_MEMORY=1 is required for GPU build."
                        [ -v PSYCLONE_HOME ] || fatal "PSYCLONE_HOME is not set. Is psyclone activated?"
                        [ -d "$PSYCLONE_HOME" ] || fatal "can't locate the PSyclone virtual environment dir."
                        if is_set_to_true NEMO_NEW_PSYCLONE_INTEGRATION; then
                            which psyclonefc >& /dev/null || fatal "for new type PSyclone integration, you have to have psyclonefc in PATH. Check the python environment."  
                            [ -v PSYCLONE__SCRIPT ] || fatal "PSYCLONE__SCRIPT is not set. Is psyclone activated?"
                            export PSYCLONE_COMPILER=$NEMO_FC
                            export NEMO_FC=psyclonefc
                            export PSYCLONE_OPTS="-l output -s ${PSYCLONE__SCRIPT}"
                            export PARALLEL_DIRECTIVES=${PSYCLONE__DIRECTIVES:=acc_offloading}
                            # NOTE: the compiler flags that PSyclone people use in their CI:
                            # export FCFLAGS="-i4 -Mr8 -O2 -Mnofma -Mnovect -g -acc=gpu -mp=gpu -gpu=mem:managed,math_uniform"
                        else
                            psyclone_makenemo_opt="-p OpenACC"
                        fi   
                    fi
                fi
            else
                local psyclone_cpu=true
                [ -v PSYCLONE__DIRECTIVES ] || psyclone_cpu=false
                is_set_to_true NEMO_OMIT_PSYCLONE && psyclone_cpu=false
                if is_set_to_true psyclone_cpu; then
					[ -v PSYCLONE_HOME ] || fatal "PSYCLONE_HOME is not set. Is psyclone activated?"
					[ -d "$PSYCLONE_HOME" ] || fatal "can't locate the PSyclone virtual environment dir."
					is_set_to_true NEMO_NEW_PSYCLONE_INTEGRATION || fatal "for PSyclone integration applied to CPU code, only new PSyclone version is allowed."
					[ -v PSYCLONE_HOME ] || fatal "PSYCLONE_HOME is not set. Is psyclone activated?"
					[ -d "$PSYCLONE_HOME" ] || fatal "can't locate the PSyclone virtual environment dir."
					which psyclonefc >& /dev/null || fatal "for new type PSyclone integration, you have to have psyclonefc in PATH. Check the python environment."
					[ -v PSYCLONE__SCRIPT ] || fatal "PSYCLONE__SCRIPT is not set. Is psyclone activated?"
					export PSYCLONE_COMPILER=$NEMO_FC
					export NEMO_FC=psyclonefc
					export PSYCLONE_OPTS="-l output -s ${PSYCLONE__SCRIPT}"
					export PARALLEL_DIRECTIVES=${PSYCLONE__DIRECTIVES}
                    [ -v PSYCLONE__FILES_TO_SKIP ] && export PSYCLONE__FILES_TO_SKIP=""
                fi
            fi
        fi
        PSYCLONE_HOME=${PSYCLONE_HOME:=/} 
        here-document arch/arch-dnb.fcm <<________EOF
            %NCDF_INC            -I$NEMO_NETCDF_FORTRAN_PATH/include
            %NCDF_LIB            -L$NEMO_NETCDF_FORTRAN_PATH/lib -L$NEMO_NETCDF_C_PATH/lib -L$NEMO_HDF5_PATH/lib -lhdf5 -lhdf5_hl -lnetcdf -lnetcdff
            %XIOS_INC            -I$NEMO_XIOS_PATH/inc
            %XIOS_LIB            $NEMO_XIOS_PATH/lib/libxios.a
            %CPP                 $NEMO_CPP
            %CC                  $NEMO_CC
            %FC                  $NEMO_FC
            %FCFLAGS             $NEMO_FCFLAGS
            %FFLAGS              %FCFLAGS
            %LD                  %FC
            %LDFLAGS             $NEMO_LDFLAGS
            %FPPFLAGS            $NEMO_FPPFLAGS
            %AR                  ar
            %ARFLAGS             rs
            %MK                  gmake
            %USER_INC            %NCDF_INC %XIOS_INC
            %USER_LIB            %NCDF_LIB %XIOS_LIB
            %PSYCLONE_HOME       $PSYCLONE_HOME
________EOF
        echo y | ./makenemo -n "$NEMO_CFG" -r ORCA2_ICE_PISCES clean_config || true
        set -x
        ./makenemo $psyclone_makenemo_opt -j $NEMO_MAKE_PARALLEL_LEVEL \
                   -r ORCA2_ICE_PISCES -n "$NEMO_CFG" -d "$NEMO_SUBCOMPONENTS" \
                   -m dnb add_key "$NEMO_KEYS_TO_ADD" del_key "$NEMO_KEYS_TO_DELETE" || fatal "makenemo failed."
        set +x
        cd $DNB_INSTALL_DIR
        [ -x $pkg-$V.src/cfgs/$NEMO_CFG/EXP00/nemo ] || fatal "makenemo failed."
    fi
    if this_mode_is_set 'i'; then
        [ -d "$pkg-$V" ] && rm -rf "$pkg-$V"
        [ -e "$pkg-$V" ] && rm -f "$pkg-$V"
        mkdir -p "$pkg-$V"
        cp -v $pkg-$V.src/cfgs/$NEMO_CFG/EXP00/nemo "$pkg-$V"
        if beginswith "$NEMO_CODEBASE" "COMMUNITY"; then
            mkdir -p "$pkg-$V/conf.ORCA2-generic"
            cp -v $pkg-$V.src/cfgs/$NEMO_CFG/EXP00/* "$pkg-$V/conf.ORCA2-generic"
            rm "$pkg-$V/conf.ORCA2-generic/nemo"
        fi
        set +u
        if beginswith "$NEMO_CODEBASE" "COMMUNITY"; then
            export NEMO_AVAILABLE_WORKLOADS=${NEMO_AVAILABLE_WORKLOADS:=ORCA2-generic}
        fi
        set -u
        cd $DNB_INSTALL_DIR
    fi
    generic_epilog
}


