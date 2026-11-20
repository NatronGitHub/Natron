
REPO_DIR=
REPO_DB_FILENAME=natron.db.tar.gz

NATRON_REPO_ADDED=0
natron_repo_add_to_pacman_conf() {
  if [[ -z ${REPO_DIR} ]]; then
    echo "Repo directory not specified. natron_repo_init must be called before this function."
    exit 1
  fi

  if [[ ${NATRON_REPO_ADDED} -ne 1 ]]; then
    if ! (grep -q 'NATRON_REPO_START' /etc/pacman.conf); then
      echo "Adding natron repository to /etc/pacman.conf"
      echo -e "#NATRON_REPO_START\n[natron]\nSigLevel = Optional TrustAll\nServer = file://${REPO_DIR}/\n#NATRON_REPO_END" >> /etc/pacman.conf

      # Refresh repositories so that the natron packages become available.
      pacman -Sy
    fi
    NATRON_REPO_ADDED=1
  fi
  return 0
}


natron_repo_remove_from_pacman_conf() {
  if (grep -q 'NATRON_REPO_START' /etc/pacman.conf); then
    echo "Removing natron repository from /etc/pacman.conf"
    sed -i -e '/^#NATRON_REPO_START/,/^#NATRON_REPO_END/{d}' /etc/pacman.conf
  fi
}

natron_repo_init() {
  if [[ -z $1 ]]; then
    echo "Repo directory not specified."
    exit 1
  fi

  if [[ -e $1 && ! -d $1 ]]; then
    echo "$1 is not a directory."
    exit 1
  fi

  REPO_DIR=$1
  if [[ ! -d ${REPO_DIR} ]]; then
    mkdir ${REPO_DIR}
  elif [[ -e "${REPO_DIR}/${REPO_DB_FILENAME}" ]]; then
    # Already have a repo database so add the repo to pacman.conf early.
    natron_repo_add_to_pacman_conf
  fi
}

natron_repo_cleanup() {
  natron_repo_remove_from_pacman_conf
}

natron_repo_add_package() {
  if [[ -z ${REPO_DIR} ]]; then
    echo "Repo directory not specified. natron_repo_init must be called before this function."
    exit 1
  fi

  local pkg_file=$1
  cp "${pkg_file}" "${REPO_DIR}"
  # In a subshell, so that the caller stays in its package directory.
  (cd "${REPO_DIR}" && repo-add -n -p "${REPO_DB_FILENAME}" "$(basename "${pkg_file}")")

  # If the natron repository hasn't been added to pacman.conf yet, do it
  # now since we should now have a valid database with at least one package
  # in it.
  if [[ ${NATRON_REPO_ADDED} -ne 1 ]]; then
    natron_repo_add_to_pacman_conf
  else
    # Sync so that the new packages are available to package builds that follow.
    pacman -Sy
  fi
}

# Natron packages with the same name as an MSYS2 package, which they replace
# (e.g. a backported fix that MSYS2 doesn't ship yet). pacman takes a package
# from the first repository in pacman.conf that has it, without comparing
# versions, and [natron] comes after the MSYS2 repositories: neither
# "pacman -S <deps>" nor "pacman -Syu" would ever pick these. Install them
# explicitly, as natron/<package>, but only if they are newer than MSYS2's, so
# that an MSYS2 rebuild or update with the fix wins automatically (and the
# Natron package should then be removed from tools/MINGW-packages).
# Names without ${MINGW_PACKAGE_PREFIX}-.
# A package that is not built for the current environment (qt5-base: CLANGARM64 only) is not in
# its [natron] repository, and is skipped.
NATRON_REPO_OVERRIDES="
freetype
qt5-base
"

natron_repo_install_overrides() {
  local msys_repo="${MSYSTEM,,}"
  local name pkg natron_ver msys_ver
  for name in ${NATRON_REPO_OVERRIDES}; do
    pkg="${MINGW_PACKAGE_PREFIX}-${name}"
    natron_ver=$(pacman -Sl natron 2>/dev/null | awk -v p="${pkg}" '$2 == p {print $3}')
    if [[ -z ${natron_ver} ]]; then
      continue
    fi
    msys_ver=$(pacman -Sl "${msys_repo}" 2>/dev/null | awk -v p="${pkg}" '$2 == p {print $3}')
    if [[ -n ${msys_ver} && $(vercmp "${natron_ver}" "${msys_ver}") -le 0 ]]; then
      echo "WARNING: ${msys_repo}/${pkg} ${msys_ver} is not older than natron/${pkg} ${natron_ver}: keeping MSYS2's."
      echo "WARNING: if it has the fix, delete tools/MINGW-packages/mingw-w64-${name}."
      continue
    fi
    echo "Installing natron/${pkg} ${natron_ver} (${msys_repo} has ${msys_ver:-none}) ..."
    pacman -S --needed --noconfirm "natron/${pkg}" || return 1
  done
  return 0
}
