function module() {
    local cmd=$1
    shift
    echo "WARNING: ignoring $cmd for modules: $@: module function seems to be unavailable"
    return 0
}

