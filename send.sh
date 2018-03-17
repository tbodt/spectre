#!/bin/bash -e
exec 3<>/dev/tcp/localhost/1234
while true; do
    echo $1 >&3
    read thing <&3
done
