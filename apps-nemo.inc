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


function mkdatalink() {
    local dir="$1"
    local file="$2"
    local oldpwd=$PWD
    [ -e "$HOME/data/$file" ] || return 0
    cd "$dir"
    [ -L "$file" ] && rm -rf "$file"
    [ -e "$file" ] || ln -s "$HOME/data/$file" .
    cd "$oldpwd"
    return 0
}

function set_path_variable_if_missing() {
    local prefix=$1    #NEMO
    local var=$2       #NETCDF_C
    local default_value=$3
    local full_var_name=${prefix}_${var}_PATH   #NEMO_NETCDF_C_PATH
    local use_prebuild_name=${prefix}_USE_PREBUILT_${var}
    if [ -z "${!use_prebuild_name}" -o "${!use_prebuild_name}" == FALSE ]; then
        [ -z "${!full_var_name}" ] || fatal "$full_var_name must not be set since $use_prebuild_name is set to TRUE";
        export ${full_var_name}=${3};
    else
        [ -z "${!full_var_name}" ] && fatal "direct path to binaries is required: $full_var_name is not set";
    fi
}


function dnb_xios() {
    generic_prolog "xios" $* || return 0	
    du_svn "https://forge.ipsl.jussieu.fr/ioserver/svn/XIOS/branchs/${pkg}-${V}"
    if this_mode_is_set 'b'; then
        set +u
        set_path_variable_if_missing NEMO HDF5 "$DNB_INSTALL_DIR/hdf5.bin"
        set_path_variable_if_missing NEMO NETCDF_C "$DNB_INSTALL_DIR/netcdf-c.bin"
        set_path_variable_if_missing NEMO NETCDF_FORTRAN "$DNB_INSTALL_DIR/netcdf-fortran.bin"
	fail_if_variable_not_defined XIOS_CCOMPILER XIOS_FCOMPILER XIOS_LINKER XIOS_CFLAGS XIOS_CPP XIOS_FPP
        set -u

        cd ${pkg}-${V}.src
        cat > arch/arch-dnb.env <<EOF
#env
export HDF5_DIR=$NEMO_HDF5_PATH
export NETCDF_DIR=$NEMO_NETCDF_C_PATH
export PNETCDF_DIR=$NEMO_NETCDF_C_PATH
EOF

        cat > arch/arch-dnb.fcm <<EOF
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
EOF

        cat > arch/arch-dnb.path <<EOF
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
EOF
        ./make_xios --netcdf_lib netcdf4_par --prod --arch dnb --job $XIOS_MAKE_PARALLEL_LEVEL || fatal "make_xios failed."
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
    fail_if_variable_not_defined NEMO_WORKLOAD_ARCHIVE NEMO_WORKLOAD_MD5 NEMO_CODEBASE

    # Source code downloads
    if beginswith "$NEMO_CODEBASE" "COMMUNITY"; then
        if beginswith "$V" "4.0"; then
            du_svn "https://forge.ipsl.jussieu.fr/nemo/svn/NEMO/releases/r4.0/r$V"
        elif beginswith "$V" "4.2"; then
            true
            fatal "getting source code: not supported version of NEMO"
            # TODO: add something working for 4.2 from github
            # go with github:
            # du_github ...
        elif beginswith "$V" "5.0"; then
            true
            fatal "getting source code: not supported version of NEMO"
            # TODO: add something working for 5.0 from github
            # go with github:
            # du_github ...
        fi
    fi

    # Debug workload downloads
    mkdatalink "$DNB_INSTALL_DIR" "$NEMO_WORKLOAD_ARCHIVE"
    if this_mode_is_set 'd'; then
        # Debug workload download procedure.
        # Avoid downloading if there are pre-downloaded files:
        cd "${pkg}.dwn"
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
	   	    # workload: for NEMO 4.0 community verson:
                    # download from https://zenodo.org/record/2640723#.ZGOnotJByV4  
                    wget -nv -O "$NEMO_WORKLOAD_ARCHIVE" "https://zenodo.org/record/2640723/files/$NEMO_WORKLOAD_ARCHIVE?download=1"
                elif beginswith "$V" "4.2"; then
		    true
		    fatal "unsupported NEMO version for debug workload downloads."
                    # TODO add workload download code for 4.2
    		elif beginswith "$V" "5.0"; then
		    true
		    fatal "unsupported NEMO version for debug workload downloads."
                    # TODO add workload download code for 5.0
                else
                    fatal "For this NEMO version, we don't know how to get the debug workload." 
                fi
            fi
        fi
        cd $DNB_INSTALL_DIR
    fi

    # Build
    if this_mode_is_set 'b'; then
        set +u
	fail_if_variable_not_defined NEMO_CFG NEMO_SUBCOMPONENTS NEMO_KEYS_TO_DELETE NEMO_KEYS_TO_ADD
        set_path_variable_if_missing NEMO HDF5 "$DNB_INSTALL_DIR/hdf5.bin"
        set_path_variable_if_missing NEMO NETCDF_C "$DNB_INSTALL_DIR/netcdf-c.bin"
        set_path_variable_if_missing NEMO NETCDF_FORTRAN "$DNB_INSTALL_DIR/netcdf-fortran.bin"
        set_path_variable_if_missing NEMO XIOS "$DNB_INSTALL_DIR/xios.bin"
	fail_if_variable_not_defined NEMO_CPP NEMO_CC NEMO_FC NEMO_FCFLAGS NEMO_LDFLAGS NEMO_FPPFLAGS
        set -u

	# Patch to enable detailed timing output
        cd ${pkg}-${V}.src
        if [ -e "./src/OCE/IOM/in_out_manager.F90" ]; then
            sed -i 's/ln_timing_detail = \.FALSE\./ln_timing_detail = .TRUE./' "./src/OCE/IOM/in_out_manager.F90"
        fi
        cat > arch/arch-dnb.fcm <<EOF
%NCDF_INC            -I$NEMO_NETCDF_FORTRAN_PATH/include
%NCDF_LIB            -L$NEMO_NETCDF_FORTRAN_PATH/lib -L$NEMO_NETCDF_C_PATH/lib -L$NEMO_HDF5_PATH/lib -lhdf5 -lhdf5_hl -lnetcdf -lnetcdff
%XIOS_INC            -I$NEMO_XIOS_PATH/inc
%XIOS_LIB            -L$NEMO_XIOS_PATH/lib -lxios -lstdc++
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
EOF
        set +u
        [ -z "$NEMO_MAKE_PARALLEL_LEVEL" ] && NEMO_MAKE_PARALLEL_LEVEL=$DNB_MAKE_PARALLEL_LEVEL
        set -u
        echo y | ./makenemo -n "$NEMO_CFG" clean_config || true
        set -x
        ./makenemo -j $NEMO_MAKE_PARALLEL_LEVEL -r ORCA2_ICE_PISCES -n "$NEMO_CFG" -d "$NEMO_SUBCOMPONENTS" -m dnb add_key "$NEMO_KEYS_TO_ADD" del_key "$NEMO_KEYS_TO_DELETE" || fatal "makenemo failed."
        set +x
        cd $DNB_INSTALL_DIR
	[ -x $pkg-$V.src/cfgs/$NEMO_CFG/EXP00/nemo ] || fatal "makenemo failed."
    fi
    if this_mode_is_set 'i'; then
        [ -d "$pkg-$V" ] && rm -rf "$pkg-$V"
        [ -e "$pkg-$V" ] && rm -f "$pkg-$V"
        mkdir -p "$pkg-$V"
        cp -v $pkg-$V.src/cfgs/$NEMO_CFG/EXP00/* "$pkg-$V"
        cd "$pkg-$V"
        tar --skip-old-files -xvf "../${pkg}.dwn/$NEMO_WORKLOAD_ARCHIVE"
        cd $DNB_INSTALL_DIR
    fi
    generic_epilog
}

