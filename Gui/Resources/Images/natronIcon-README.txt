# conversion from svg to pdf:
    rsvg-convert -f pdf -o natronIcon.pdf natronIcon.svg

alternatively, the following should work, but prefer the method above:

    inkscape natronIcon.svg --export-pdf=natronIcon.pdf

# icon generation:

density 300 gives a 1708x1708 image, which is large enough for all chosen resolutions.

    convert -background none -density 300 natronIcon.svg -resize 256x256 natronIcon256_linux.png
    optipng -o 7 natronIcon256_linux.png

for windows, see http://www.imagemagick.org/Usage/thumbnails/#favicon

    convert -background none -density 300 natronIcon.svg -define icon:auto-resize=256,64,48,32,16 natronIcon256_windows.ico

for macOS (must be run on a mac), see https://developer.apple.com/library/content/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Optimizing/Optimizing.html#//apple_ref/doc/uid/TP40012302-CH7-SW2

    mkdir natronIcon.iconset
    convert -background none -density 300 natronIcon.svg -resize 16x16 natronIcon.iconset/icon_16x16.png
    convert -background none -density 300 natronIcon.svg -resize 32x32 natronIcon.iconset/icon_16x16@2x.png
    convert -background none -density 300 natronIcon.svg -resize 32x32 natronIcon.iconset/icon_32x32.png
    convert -background none -density 300 natronIcon.svg -resize 64x64 natronIcon.iconset/icon_32x32@2x.png
    convert -background none -density 300 natronIcon.svg -resize 64x64 natronIcon.iconset/icon_64x64.png
    convert -background none -density 300 natronIcon.svg -resize 128x128 natronIcon.iconset/icon_64x64@2x.png
    convert -background none -density 300 natronIcon.svg -resize 128x128 natronIcon.iconset/icon_128x128.png
    convert -background none -density 300 natronIcon.svg -resize 256x256 natronIcon.iconset/icon_128x128@2x.png
    convert -background none -density 300 natronIcon.svg -resize 256x256 natronIcon.iconset/icon_256x256.png
    convert -background none -density 300 natronIcon.svg -resize 512x512 natronIcon.iconset/icon_256x256@2x.png
    convert -background none -density 300 natronIcon.svg -resize 512x512 natronIcon.iconset/icon_512x512.png
    convert -background none -density 300 natronIcon.svg -resize 1024x1024 natronIcon.iconset/icon_512x512@2x.png
    optipng -o 7 natronIcon.iconset/icon_*png
    iconutil -c icns natronIcon.iconset
    
