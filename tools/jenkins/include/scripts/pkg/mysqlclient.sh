#!/bin/bash
if $WITH_MARIADB; then

# Install mysqlclient (MySQL/MariaDB connector)
# mysqlclient is a fork of MySQL-python. It adds Python 3 support and fixed many bugs.
# see https://pypi.python.org/pypi/mysqlclient
MYSQLCLIENT_VERSION_PY3=2.0.3
MYSQLCLIENT_VERSION_PY2=1.4.6
#dobuild && echo "python${PY2_VERSION_SHORT}"
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/MySQLdb" ] || [ "$("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import MySQLdb; print(MySQLdb.__version__.split(' ', 1)[0])")" != "${MYSQLCLIENT_VERSION_PY2}" ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary mysqlclient mysqlclient=="${MYSQLCLIENT_VERSION_PY2}"
    end_build
fi
# in MySQLsb >= 2.0, the version is in MySQLdb.release
if build_step && { force_build || { [ ! -d "$SDK_HOME/lib/python${PY3_VERSION_SHORT}/site-packages/MySQLdb" ] || [ "$("$SDK_HOME/bin/python${PY3_VERSION_SHORT}" -c "import MySQLdb.release; print(MySQLdb.release.__version__.split(' ', 1)[0])")" != "${MYSQLCLIENT_VERSION_PY3}" ]; }; }; then
    start_build
    ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --no-binary mysqlclient mysqlclient=="${MYSQLCLIENT_VERSION_PY3}"
    end_build
fi

fi
