#/bin/bash
#
# Natron Linux hotfix script
# Will fix broken symlinks in a release
#

if [ $# -ne 1 ]; then
    echo "Natron hotfix script"
    echo ""
    echo "Usage: ${0} <Natron_installation_folder>"
    exit 0
fi

TMP_DIR="/tmp/Natron-hotfix.$$"
NATRON_DIR=`readlink -f ${1}`
LCMS_SO_32="liblcms2-Linux-x86_32bit.tgz"
LCMS_SO_64="liblcms2-Linux-x86_64bit.tgz"
LCMS_URL="https://github.com/NatronGitHub/Natron/releases/download/v2.3.14"
LCMS_SO_32_URL="${LCMS_URL}/${LCMS_SO_32}"
LCMS_SO_64_URL="${LCMS_URL}/${LCMS_SO_64}"
LCMS_SO_URL=${LCMS_SO_32_URL}
LCMS_SO=${LCMS_SO_32}
if [ `uname -m` == "x86_64" ]; then
    LCMS_SO_URL=${LCMS_SO_64_URL}
    LCMS_SO=${LCMS_SO_64}
fi

if [ ! -f "${NATRON_DIR}/bin/Natron" ]; then
    echo "Not a Natron installation folder."
    exit 1
fi

# Find broken symlinks
BROKEN_SYMLINKS=`cd ${NATRON_DIR}; find . -type l ! -exec test -e {} \; -print`
for symlink in ${BROKEN_SYMLINKS}; do
    symlink_path=`echo ${symlink} | sed 's#./#'${NATRON_DIR}'/#'`
    symlink_file=`basename ${symlink_path} | sed 's/\[0-9]/1/'` # replace [0-9] with 1
    symlink_dir=`dirname ${symlink_path}`

    # Delete all symlinks with [0-9]
    if [[ $symlink == *"[0-9]"* ]]; then
        rm -f "${symlink_path}" || exit 1
    fi

    # Download missing lcms
    if [ "${symlink_file}" == "liblcms2.so.2" ] && [ ! -e "${NATRON_DIR}/lib/${symlink_file}" ]; then
        if [ ! -d "$TMP_DIR" ]; then
            mkdir -p "$TMP_DIR" || exit 1
        fi
        cd "$TMP_DIR" || exit 1
        wget --quiet ${LCMS_SO_URL} || exit 1
        tar xf ${LCMS_SO} || exit 1
        cp ${symlink_file} "${NATRON_DIR}/lib/" || exit 1        
    fi

    # Find and fix symlinks
    cd "${symlink_dir}" || exit 1
    if [ -e "../../../../../lib/${symlink_file}" ]; then
        ln -sf "../../../../../lib/${symlink_file}" ${symlink_file} || exit 1
    elif [ -e "../../IO.ofx.bundle/Libraries/${symlink_file}" ]; then
        ln -sf "../../IO.ofx.bundle/Libraries/${symlink_file}" ${symlink_file} || exit 1
    elif [ -e "../../Arena.ofx.bundle/Libraries/${symlink_file}" ]; then
        ln -sf "../../Arena.ofx.bundle/Libraries/${symlink_file}" ${symlink_file} || exit 1
    # else
        # can't find anything to symlink, just delete symlink
        # rm -f "${symlink_file}"
    fi
done

if [ -d "${TMP_DIR}" ]; then
    rm -rf "${TMP_DIR}"
fi
