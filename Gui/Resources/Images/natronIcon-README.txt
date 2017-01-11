conversion from svg to pdf:
rsvg-convert -f pdf -o natronIcon.pdf natronIcon.svg

alternatively, the following should work, but prefer the method above:
inkscape natronIcon.svg --export-pdf=natronIcon.pdf
