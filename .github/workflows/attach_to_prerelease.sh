#!/bin/bash
# Attach files to the GitHub pre-release of TAG, creating the pre-release if it doesn't exist yet
# (gh reads GH_TOKEN and GH_REPO from the environment). Used on pre-release tags by
# linux_packages.yml, macos_x86_64.yml and build_installer.yml, which run at the same time: if
# another workflow creates the release first, "gh release create" fails and the release is used.
#
#   attach_to_prerelease.sh TAG FILE...
set -euo pipefail

[ $# -ge 2 ] || { sed -n 2,7p "$0"; exit 2; }
tag=$1
shift
gh release view "$tag" > /dev/null 2>&1 ||
    gh release create "$tag" --verify-tag --prerelease --title "$tag" --notes "Natron $tag (pre-release)" ||
    gh release view "$tag" > /dev/null
gh release upload "$tag" --clobber "$@"
