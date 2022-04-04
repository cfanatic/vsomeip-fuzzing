#!/usr/bin/env bash

function fuzz {
    echo "Info: Starting the fuzz process for $1 seconds"
    afl-fuzz -V "$1" -D -i afl_input/ -o afl_output/ ./fuzzing @@
}

function report {
    echo "Info: Starting the report process"
    afl-cov -d afl_output/ --coverage-cmd "./fuzzing AFL_FILE" --code-dir ../ --overwrite
}

main() {
    case "$1" in
        -fuzz) fuzz "$2";;
        -report) report;;
        *) echo "Error: Parameter '-fuzz' or '-report' missing"; exit 1;;
    esac
}

main "$@"
