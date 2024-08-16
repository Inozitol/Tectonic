#!/bin/sh

ktx create --zstd 18 --format R8G8B8A8_UNORM --assign-oetf linear --assign-primaries bt709 --layers 6 ${1}/xpos.png ${1}/xneg.png ${1}/ypos.png ${1}/yneg.png ${1}/zpos.png ${1}/zneg.png ${1}/color.ktx2
