
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
  cp ${pkg_file} ${REPO_DIR}
  cd ${REPO_DIR}
  repo-add -n -p ${REPO_DB_FILENAME} ${pkg_file}

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