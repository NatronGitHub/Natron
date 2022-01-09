#!/bin/bash

# install psycopg2 (PostgreSQL connector)
# see https://pypi.python.org/pypi/psycopg2
PSYCOPG2_VERSION_PYTHON2=2.8.6
PSYCOPG2_VERSION_PYTHON3=2.9.3
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/psycopg2" ] || [ "$("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import psycopg2; print(psycopg2.__version__.split(' ', 1)[0])")" != "${PSYCOPG2_VERSION_PYTHON2}" ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary psycopg2 psycopg2=="${PSYCOPG2_VERSION_PYTHON2}"
    end_build
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY3_VERSION_SHORT}/site-packages/psycopg2" ] || [ "$("$SDK_HOME/bin/python${PY3_VERSION_SHORT}" -c "import psycopg2; print(psycopg2.__version__.split(' ', 1)[0])")" != "${PSYCOPG2_VERSION_PYTHON3}" ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --no-binary psycopg2 psycopg2=="${PSYCOPG2_VERSION_PYTHON3}"
    end_build
fi
