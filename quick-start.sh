#!/bin/bash

set -e

echo -e "This is a quick-start build script for the Teechain/Keystone, it
will clone and build all the necessary parts to run the demo
server/applcation and client on a RISC-V platform (ex: qemu). Please
ensure you have cloned keystone completely and that you have fully
built the sdk tests and run them successfully in qemu.

You must set KEYSTONE_SDK_DIR to the install directory of Keystone SDK.

You must have the riscv64 gcc on-path as well. (e.g. run
'source source.sh' in the Keystone directory.

If you have already started building libsodium/etc, it is not
recommended to use this script."
read -r -p "Continue? [Y/n] " response
response=${response,,}
if [[ "$response" =~ ^(no|n)$ ]]
then
    exit 0
fi

# Check location/tools
if [[ ! -v KEYSTONE_SDK_DIR ]]
then
    echo "KEYSTONE_SDK_DIR not set! Please set this to the location where Keystone SDK has been installed."
    exit 0
fi

if [[ ! $(command -v riscv64-unknown-linux-gnu-gcc) ]]
then
    echo "No riscv64 gcc available. Make sure you've run \"source source.sh\" in the Keystone directory (or equivalent.)";
    exit 0
fi

TEECHAIN_DIR=$(pwd)

set -e

git submodule update --init

# Build libbtc
cd libbtc
./autogen.sh
./configure --host=riscv64-unknown-linux-gnu --disable-wallet --disable-tools --disable-net
make
cd ..

# Clone, checkout, and build the server libsodium
cd libsodium
./autogen.sh
./configure --host=riscv64-unknown-linux-gnu --disable-ssp --disable-asm --without-pthreads
make
cd ..

# cd $KEYSTONE_SDK_DIR/../
# git apply $TEECHAIN_DIR/patches/0001-add-syscall-interface.patch
# cd build
# make
# make install
# cd $TEECHAIN_DIR

# update source.sh
echo "export LIBBTC_DIR=$(pwd)/libbtc" > ./source.sh
echo "export LIBSODIUM_DIR=$(pwd)/libsodium/src/libsodium" >> ./source.sh
echo "Libbtc and Libsodium have been fully setup"
echo ""
echo " * Notice: run the following command to update enviroment variables *"
echo ""
echo "           source ./source.sh"
echo ""
