#!/bin/sh
set -e
sudo chown -R artingl:staff .
make clean
ssh parallels@10.211.55.13 "cd /media/psf/Home/stuff/workspace/local/Inari && make build_kernel -j6"
make test_serial

