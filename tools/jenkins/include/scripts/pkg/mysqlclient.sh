#!/bin/bash

# Install mysqlclient (MySQL/MariaDB connector)
# mysqlclient is a fork of MySQL-python. It adds Python 3 support and fixed many bugs.
# see https://pypi.python.org/pypi/mysqlclient
MYSQLCLIENT_VERSION=1.3.12
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/MySQLdb" ] || [ $("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import MySQLdb; print MySQLdb.__version__") != ${MYSQLCLIENT_VERSION} ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary mysqlclient mysqlclient=="${MYSQLCLIENT_VERSION}"
    end_build
fi
