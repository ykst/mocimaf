#!/bin/bash
pushd () {
    command pushd "$@" > /dev/null
}
popd () {
    command popd "$@" > /dev/null
}

pushd roms; roms="`pwd`"; popd
pushd ..; nes="`pwd`/mocimaf"; popd

trap 'echo "*** test cancelled by user interrupt"; exit 1' SIGINT
