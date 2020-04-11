#!/bin/bash

# install psycopg2 (PostgreSQL connector)
# see https://pypi.python.org/pypi/psycopg2
PSYCOPG2_VERSION=2.8.5
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/psycopg2" ] || [ $("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import psycopg2; print(psycopg2.__version__.split(' ', 1)[0])") != ${PSYCOPG2_VERSION} ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary psycopg2 psycopg2=="${PSYCOPG2_VERSION}"
    end_build
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY3_VERSION_SHORT}/site-packages/psycopg2" ] || [ $("$SDK_HOME/bin/python${PY3_VERSION_SHORT}" -c "import psycopg2; print(psycopg2.__version__.split(' ', 1)[0])") != ${PSYCOPG2_VERSION} ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --no-binary psycopg2 psycopg2=="${PSYCOPG2_VERSION}"
    end_build
fi
