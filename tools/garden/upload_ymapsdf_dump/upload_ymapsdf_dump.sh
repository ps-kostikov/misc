#!/bin/bash -ve

REGION=russia
TARGZ=$1
DATE=$2
STORAGE_FILE=storage.dat
MODULES_DIRECTORY=modules.d

mkdir -p modules.d
ln -s /usr/lib/yandex/maps/garden/modules.d/ymapsdf_load.py modules.d/ymapsdf_load.py
ln -s /usr/lib/yandex/maps/garden/modules.d/ymapsdf_src.py modules.d/ymapsdf_src.py

mkdir -p tmp
mkdir -p work
SRC_DIR='src_'$REGION'_yandex_'$DATE
mkdir -p work/$SRC_DIR
ln -s `readlink -f $TARGZ` work/$SRC_DIR/dump.tar.gz

GARDEN_CONF_PATH=configs:/etc/yandex/maps/garden/ GARDEN_IGNORE_PKG_VERSIONS=1 execute_garden_modules --storage-path=$STORAGE_FILE --init-modules=ymapsdf_src --target-modules=ymapsdf_load --modules-directory=$MODULES_DIRECTORY --debug --russia=$DATE

rm -f $STORAGE_FILE
rm -f $STORAGE_FILE'.lock'
rm -rf work
rm -rf tmp
rm -rf modules.d

