#!/bin/bash

NATRON_DIR=
NATRON_REPO_DIR=
if [[ -z $1 || -z $2 ]]; then
  echo "Usage: $(basename $0) <natron_directory> <natron_repo_directory>"
  exit 1
else
  NATRON_DIR=$(realpath $1)
  NATRON_REPO_DIR=$(realpath $2)
fi

source ${NATRON_DIR}/tools/MINGW-packages/natron_repo_common.sh

if [[ ! -d ${NATRON_REPO_DIR} ]]; then
    mkdir ${NATRON_REPO_DIR}
fi

cd ${NATRON_REPO_DIR}

WINDOWS_PACMAN_REPO_VERSION=`cat ${NATRON_DIR}/tools/MINGW-packages/windows_pacman_repo_version.txt`
ZIP_FILENAME="natron_package_repo-${WINDOWS_PACMAN_REPO_VERSION}.zip"

# Try the repository the build is running from before upstream, so a fork that
# has rebuilt the packages can publish them on its own release instead of being
# limited to whatever upstream last shipped. Falling straight through to the
# upstream release keeps behaviour identical for NatronGitHub itself.
UPSTREAM_REPO_URL="https://github.com/NatronGitHub/Natron"
REPO_RELEASE_PATH="releases/download/windows-mingw-package-repo/${ZIP_FILENAME}"
REPO_URLS=()
if [[ -n "${GITHUB_REPOSITORY:-}" ]]; then
    REPO_URLS+=("${GITHUB_SERVER_URL:-https://github.com}/${GITHUB_REPOSITORY}")
fi
if [[ "${REPO_URLS[0]:-}" != "${UPSTREAM_REPO_URL}" ]]; then
    REPO_URLS+=("${UPSTREAM_REPO_URL}")
fi
for repo_url in "${REPO_URLS[@]}"; do
    echo "Trying ${repo_url}/${REPO_RELEASE_PATH}"
    # -O always writes to ${ZIP_FILENAME} (never ${ZIP_FILENAME}.1). wget leaves
    # an empty or truncated file behind on failure, and a truncated zip can also
    # come from an interrupted transfer, so remove it unless it is a valid zip.
    if wget --no-verbose -O "${ZIP_FILENAME}" "${repo_url}/${REPO_RELEASE_PATH}" && unzip -tq "${ZIP_FILENAME}" > /dev/null; then
        echo "Fetched ${ZIP_FILENAME} from ${repo_url}"
        break
    fi
    rm -f "${ZIP_FILENAME}"
done

if [[ -e ${ZIP_FILENAME} ]]; then
    unzip ${ZIP_FILENAME}
else
    echo "Failed to fetch ${ZIP_FILENAME}"

    echo "Building pacman repo locally."

    # install necessary dependencies for building pacman repo.
    pacman -S --needed --noconfirm git base-devel

    ${NATRON_DIR}/tools/MINGW-packages/build_natron_package_repo.sh ${NATRON_REPO_DIR}
fi

UNIX_NATRON_REPO_DIR=`cygpath -u ${NATRON_REPO_DIR}`
natron_repo_init ${UNIX_NATRON_REPO_DIR}

# Natron packages that replace MSYS2's (e.g. freetype with the ULABS fix, #1287)
# are never picked by pacman from the [natron] repository on its own.
natron_repo_install_overrides || exit 1
