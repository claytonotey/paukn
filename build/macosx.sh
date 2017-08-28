#!/bin/bash

ROOT=/Developer/SDKs/MacOSX10.4u.sdk

VSTROOT=../../vst2.4
export VSTSDK_DIR="$VSTROOT/sdk"
export VSTPLUG_DIR="$VSTROOT/plugin"
export VSTGUI_DIR="$VSTROOT/vstgui"
export VST_CFLAGS="-I$VSTSDK_DIR -I$VSTPLUG_DIR -I$VSTGUI_DIR"
export VST_LIBS="-framework Carbon -framework QuickTime -framework System -framework ApplicationServices"


./configure --disable-universal_binary --enable-vst && make
#./configure --enable-vst && make clean && make
