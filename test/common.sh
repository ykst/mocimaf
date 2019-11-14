#!/bin/bash

. env.sh

patterns=()
if [[ ! $1 ]]; then
    patterns+=("specs/*")
else
    for pattern in "$@"; do
        patterns+=("specs/${pattern:-*}")
    done
fi

checks="Frame   Cycle   Video   Audio   Step"
errors=()
targets=()
for d in ${patterns[*]}; do
    if [[ -d "$d" ]]; then
        targets+=("$d")
    fi
done
total=${#targets[*]}

epilogue () {
    echo "---------------------------------"
    error_cnt=${#errors[*]}
    if  [[ $error_cnt > 0 ]]; then
        if [[ -x `command -v say` ]]; then
            say -v Victoria "failed $error_cnt tests"
        fi
        printf "*** failed %d tests (total %d)\n" $error_cnt $total
        echo ${errors[*]}
        exit $error_cnt
    else
        if [[ $MOCIMAF_TEST_WITH_SOUND && -x `command -v say` ]]; then
            say -v Victoria "passed all tests"
        fi
        printf "🌸 passed all tests (total %d)\n" $total
        exit 0
    fi
}
