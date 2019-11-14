#!/bin/bash



pushd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null
. common.sh

# show header
printf  "======== Regression Test ========\n%32s %s    Time[ms]\n" ""  "$checks"

# go!
count=1

for d in ${targets[*]}; do
    pushd "$d"
        spec="`pwd`"
        name=${d##specs/}

        printf "%32s " "$name($count/$total)"
        eval "`cat ./shell`" | awk -v check_list="$checks" '{ result[$1] = $2 } END {
            while (( getline < "./expected") > 0) {
                split($0, cols)
                expected[cols[1]] = cols[2]
            }
            n = split(check_list, checks)
            for (i = 1; i <= n; ++i) {
                check = "@" checks[i]
                if (!result[check]) {
                    printf "        "
                    failed = 1
                } else if (expected[check] == result[check]) {
                    printf "✔︎       "
                } else {
                    printf "❌      "
                    failed = 1
                }
            }
            print result["#Time"]
            if (failed) { exit 1 }
        }' || errors+=($name)

        let ++count
    popd
done

epilogue
