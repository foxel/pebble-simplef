#!/bin/sh

convert image.png -compose Screen \
    \( -clone 0 image-i.png \) \
    -delete 0 -set delay 300 \
    -layers Optimize image.gif
