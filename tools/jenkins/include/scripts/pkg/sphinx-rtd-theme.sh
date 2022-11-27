#!/bin/bash

# install sphinx-rtd-theme (Read the Docs theme for Sphinx)
# see https://pypi.org/project/sphinx-rtd-theme/
SPHINXRTDTHEME_VERSION_PYTHON3=1.1.1
SPHINXRTDTHEME_VERSION_PYTHON2=0.4.3 # 0.5.0 only supports Python 3
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/sphinx_rtd_theme" ] || [ "$("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import sphinx_rtd_theme; print(sphinx_rtd_theme.__version__.split(' ', 1)[0])")" != "${SPHINXRTDTHEME_VERSION_PYTHON2}" ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary sphinx_rtd_theme sphinx_rtd_theme=="${SPHINXRTDTHEME_VERSION_PYTHON2}"
    end_build
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY3_VERSION_SHORT}/site-packages/sphinx_rtd_theme" ] || [ "$("$SDK_HOME/bin/python${PY3_VERSION_SHORT}" -c "import sphinx_rtd_theme; print(sphinx_rtd_theme.__version__.split(' ', 1)[0])")" != "${SPHINXRTDTHEME_VERSION_PYTHON3}" ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --no-binary sphinx_rtd_theme sphinx_rtd_theme=="${SPHINXRTDTHEME_VERSION_PYTHON3}"
    end_build
fi
