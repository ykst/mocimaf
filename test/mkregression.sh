#!/bin/bash
name=$1
rom=$2

pushd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null

. env.sh

spec="specs/$name"
mkdir -p "$spec"

if [[ -f "$spec/rec" ]]; then
    while true; do
        read -p "playlog already exists. Do you want to override it? [y]" yn
        case $yn in
            [Yy]* ) break;;
            *) exit;;
        esac
    done
    rm -f "$spec/rec"
fi

if [[ ! -f "$roms/$rom" ]]; then
    echo "$rom not found under $roms"
    exit 1
fi

shell="\$nes \"\$roms/$rom\" -p \"\$spec/rec\" -q"
run="\$nes \"\$roms/$rom\" -r \"\$spec/rec\""
echo $shell > "$spec/shell"
eval $run | grep "^@" > "$spec/expected"
echo "done"
