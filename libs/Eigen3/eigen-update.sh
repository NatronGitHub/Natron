#!/bin/sh
# Refresh the vendored Eigen tree from upstream.
#
# Eigen left Bitbucket and Mercurial behind; it lives in git on GitLab now.
# The tree is replaced rather than copied over, so that files upstream has
# deleted do not linger. Overlaying is how the Eigen2 compatibility modules
# survived here for years after 3.3 removed them.

set -e

EIGEN_VERSION=3.4.1
# the commit that the tag must point to (a moved tag fails the update)
EIGEN_COMMIT=d71c30c47858effcbd39967097a2d99ee48db464
EIGEN_REPO=https://gitlab.com/libeigen/eigen.git

echo "*** Eigen update utility"
echo "*** This replaces the whole Eigen/ directory with upstream ${EIGEN_VERSION}"

if [ "x$1" = "x--i-really-know-what-im-doing" ]; then
    echo "Proceeding as requested by command line ..."
else
    echo "*** Please run again with --i-really-know-what-im-doing ..."
    exit 1
fi

cd "$(dirname "$0")"

rm -rf eigen-checkout
git clone --depth 1 --branch "${EIGEN_VERSION}" "${EIGEN_REPO}" eigen-checkout
actual=$(git -C eigen-checkout rev-parse HEAD)
if [ "$actual" != "$EIGEN_COMMIT" ]; then
    echo "*** Error: tag ${EIGEN_VERSION} is commit $actual, expected ${EIGEN_COMMIT}" >&2
    rm -rf eigen-checkout
    exit 1
fi

rm -rf Eigen
cp -r eigen-checkout/Eigen Eigen
find Eigen -type f -name CMakeLists.txt -delete
rm -rf eigen-checkout

echo "*** Updated to ${EIGEN_VERSION}. Update the version and commit in README.Natron too."
