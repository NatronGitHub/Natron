# !/usr/bin/sh

thread=4
natron_src='./'
qmake="qmake-qt4"
compile_type='Release'

# entorno para gcc 8.3
source /opt/rh/devtoolset-8/enable
# -----------------

cd $natron_src

pkill -9 Natron
rm App/Natron
rm /opt/Natron2/bin/Natron

# Compilacion
$qmake -r CONFIG+="$compile_type" ./Project.pro
make -j $thread
# -------------------

# copia Natron compilado a la carpeta de la instalacion de Natron

cp ./App/Natron /opt/Natron2/bin/Natron
/opt/Natron2/bin/Natron
# -------------------