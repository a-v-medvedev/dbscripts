DNB_DBSCRIPTSDIR=${DNB_DBSCRIPTSDIR:-"./dbscripts"}
source $DNB_DBSCRIPTSDIR/base.inc
source $DNB_DBSCRIPTSDIR/funcs.inc
source $DNB_DBSCRIPTSDIR/cache.inc
source $DNB_DBSCRIPTSDIR/compchk.inc
source $DNB_DBSCRIPTSDIR/envchk.inc
source $DNB_DBSCRIPTSDIR/db.inc
source $DNB_DBSCRIPTSDIR/apps.inc
which module >& /dev/null || [ `type -t module`"" == 'function' ] || source $DNB_DBSCRIPTSDIR/module.inc

