#!/bin/bash
#
# Script to launch jenkins.sh on osx
#
set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
set -x # Print commands and their arguments as they are executed.

# Ensure we have the build master ssh key to clone/update repository
# Note: This is now done by keychain (see ~/.bash_profile)
#. ensure-ssh-identity.sh
if [ -f "$HOME/.ssh/id_rsa_build_master" ]; then
    keychainstatus=0
    eval `keychain -q --eval --agents ssh id_rsa_build_master` || keychainstatus=1
    if [ $keychainstatus != 0 ]; then
        echo "Restarting ssh-agent"
        keychain -k
        eval `keychain --eval --agents ssh id_rsa_build_master`
    fi
fi

# Config git if not done so far
git config --global user.email "support@natron.fr"
git config --global user.name "Natron CI"

cd ~/Data/natron-support

BRANCHNAME=$(git branch | grep '^\*' | awk '{print $2}')
if [ "$BRANCHNAME" != "jenkins" ]; then
    git checkout jenkins
fi
git pull --rebase --autostash
git submodule update -i --recursive
cd buildmaster

export MKJOBS=2

# Exec command in same shell to inherit all variables set by jenkins
. launchBuildMain.sh

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
