#!/bin/bash

OpenWrtSDK='/tmp/backfire/build_dir/target-mips_r2_uClibc-0.9.30.1/OpenWrt-SDK-ar71xx-for-Linux-x86_64-gcc-4.3.3+cs_uClibc-0.9.30.1'
PackageDir="$OpenWrtSDK/package"
AppPath=$(pwd)
Apps=$(ls -d */ | sed -e 's/\///g')

echo "OpenWrtSDK=$OpenWrtSDK"
echo "PackageDir=$PackageDir"
for i in $Apps; do
        cp -rf $i "$PackageDir/";
done

cd "$OpenWrtSDK"
make package/compile V=99 clean
make package/compile V=99

mkdir -p $AppPath/../dist/
for i in $Apps; do
        cp -rf bin/ar71xx/packages/$i_*.ipk  $AppPath/../dist/;
done;

cd $AppPath

