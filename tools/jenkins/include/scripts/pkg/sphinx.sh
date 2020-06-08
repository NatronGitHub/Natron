#!/bin/bash

# install Sphinx (Python documentation generator)
# see https://pypi.org/project/sphinx/
SPHINX_VERSION_PYTHON3=2.4.4 # 3.0.3 is the latest but must check compatibility
SPHINX_VERSION_PYTHON2=1.8.5 # last version that supports Python 2.7
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/sphinx" ] || [ $("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import sphinx; print(sphinx.__version__.split(' ', 1)[0])") != ${SPHINX_VERSION_PYTHON2} ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary sphinx sphinx=="${SPHINX_VERSION_PYTHON2}"
    end_build
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY3_VERSION_SHORT}/site-packages/sphinx" ] || [ $("$SDK_HOME/bin/python${PY3_VERSION_SHORT}" -c "import sphinx; print(sphinx.__version__.split(' ', 1)[0])") != ${SPHINX_VERSION_PYTHON3} ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --no-binary sphinx sphinx=="${SPHINX_VERSION_PYTHON3}"
    end_build
fi
