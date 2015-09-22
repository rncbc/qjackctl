#!/bin/bash
# This script is supposed to generate icons from blender files. At the time 
# of writing blender does not have command line parameters to control size of
# the output and limited possibilities setting an output file name. To render
# in different resolutions a python script might be an option or file output
# nodes in the compositor

blender --background qjackctl.blend --render-output //####_qjackctl.png --render-frame 1
mv 0001_qjackctl.png qjackctl.png
