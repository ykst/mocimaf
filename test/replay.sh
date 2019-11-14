#!/bin/bash

pushd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null
. common.sh

printf "replaying tests...\n"

# go!
count=1

for d in ${targets[*]}; do
    pushd "$d"
        spec="`pwd`"
        name=${d##specs/}

        printf "%s\n" "($count/$total)$name"
        (eval "`cat shell` -q -u" && echo "✔︎") || errors+=($name)

        let ++count
    popd
done

epilogue
