
* Download Natron SDK: msys2-common.7z and mingw64.7z/mingw32.7z
* Extract to c:\msys

Setup jenkins slave to execute batch command:

```
set MSYSTEM=MINGW64
echo %GIT_URL% > git_url.txt
echo %GIT_COMMIT% > git_commit.txt
echo %GIT_BRANCH% > git_branch.txt
C:\msys\usr\bin\bash.exe --login
```

The build proc must be added to or launched from $HOME/.bash_profile, example:

```
gcc -v
uname -a

export NAT_URL=`cat /c/builds/workspace/natron-build-win-64/git_url.txt|sed 's# ##g'`
export NAT_COMMIT=`cat /c/builds/workspace/natron-build-win-64/git_commit.txt|sed 's# ##g'`
export NAT_BRANCH=`cat /c/builds/workspace/natron-build-win-64/git_branch.txt|sed 's#origin/##;s# ##g'`
echo $NAT_URL
echo $NAT_COMMIT
echo $NAT_BRANCH

eval `ssh-agent`
ssh-add $HOME/id_rsa_build_master

cd $HOME

if [ -d "$HOME/natron-support" ]; then
  ( cd natron-support;
    git pull
    git submodule update -i --recursive
  )
else
  git clone git+ssh://natron-ci@scm.gforge.inria.fr/gitroot/natron-support/natron-support.git
  ( cd natron-support
    git submodule update -i --recursive
  )
fi

if [ ! -z "$NAT_URL" ] && [ ! -z "$NAT_COMMIT" ] && [ ! -z "$NAT_BRANCH" ]; then
  cd $HOME/natron-support/buildmaster
  env BIT=64 NATRON_LICENSE=GPL BUILD_CONFIG=SNAPSHOT GIT_BRANCH=master GIT_COMMIT="" sh include/scripts/build-plugins.sh
  env BIT=64 NATRON_LICENSE=GPL BUILD_CONFIG=SNAPSHOT GIT_BRANCH=$NAT_BRANCH GIT_COMMIT=$NAT_COMMIT sh include/scripts/build-natron.sh
fi
```

