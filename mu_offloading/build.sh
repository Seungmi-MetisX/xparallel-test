#!/bin/bash

this_path=$(readlink -f "$0")
this_dir=$(dirname "$this_path")
this_file=$(basename "$this_path")
this_name=${this_file%.*}

set -exu

rm -rf $this_dir/build
mkdir -p $this_dir/build/debug/kernel

# build kernel
cd $this_dir/build/debug/kernel
cmake ../../../kernel
cmake --build .

cp /home/smshin/workspace/sdk/mu/out/inverted_index_flat_kernel* $this_dir/build/debug/kernel/.
cp /home/smshin/workspace/sdk/mu/out/inverted_index_kernel* $this_dir/build/debug/kernel/.

# build host and cpp xparallel
cd $this_dir/build/debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# build rust xparallel (not included in crate yet)
cd $this_dir/../crates/xparallel
# cargo build

# copy after xparallel build
cp $this_dir/build/debug/helper/libmetisx_helper.so $this_dir/../target/debug
cp $this_dir/build/debug/xparallel/libxparallel.so $this_dir/../target/debug
