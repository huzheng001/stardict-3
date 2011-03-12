#!/bin/sh
# Create images for StarDict, like application icon, tray icons, Acrobat plugin icons
# StarDict icons may be created from a single multilayer image.
# Most layers are used only for specific icon types, but images of two books are present 
# on all icons. This script deletes layers, resize image and change color depth to create icons.
# 
# Prerequisites
# - Install GIMP - http://www.gimp.org (this plugin was tested with GIMP 2.6)
# - Make sure that GIMP-Python scripting extension (http://www.gimp.org/docs/python/index.html) is installed (how?). 
# - Install python 2.x
# - Copy or symlink stardict_images.py from StarDict tools to ~/.gimp-2.6/plug-ins/
# 
# Usage
# stardict_images.sh path/to/stardict.xcf path/to/stardict/root/directory
# You may find stardict.xcf in StarDict repository at stardict/pixmaps/stardict.xcf.
# Note that stardict.xcf is not included in source tarballs.
# If stardict.xcf is located in source tree, you may omit the second parameter, 
# it will be calculated automatically.
# Otherwise the second parameter must point to the root of stardict source tree, 
# this is where icons will be created.
main_image="$1"
root_dir=
if [ $# -gt 1 ]; then
root_dir="$2"
fi
gimp -idf -b "(python-fu-stardict-images RUN-NONINTERACTIVE \"$main_image\" \"$root_dir\")" -b '(gimp-quit 0)'
