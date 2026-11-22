# shellcheck shell=bash disable=SC2034 # sourced; the variables are used by the sourcing scripts
# macOS release settings (sourced by tools/jenkins/common.sh and the tools/tart scripts): the
# single place to change when the oldest supported macOS changes.
#
# Maintainer decisions: 2026-11-18, Natron 2.6 for macOS runs on macOS 14 (Sonoma) or later;
# 2026-11-21, macOS 14 is the distribution baseline: the release is built on macOS 14, in a tart
# VM made from the stock cirruslabs Sonoma image, with only the Command Line Tools that have the
# macOS 14 SDK (no Xcode). GitHub CI uses macos-15 runners only because there are no macOS 14 ones.
#
# Changing the target also means: a new BASE_IMAGE and CLT_VERSION below, rebuilding the SDK image
# (tools/tart/natron-sdk.sh create), and the system requirements in the User Guide.

# Oldest macOS the release runs on: MACOSX_DEPLOYMENT_TARGET of release builds (NATRON_MACOS_MIN,
# see common.sh), macosx_deployment_target in MacPorts, and the default target of check-minos.sh.
NATRON_MACOS_RELEASE_MIN=14.0
# Command Line Tools installed in the SDK image (tools/tart/provision.sh, step_clt): 15.3 has the
# macOS 14.4 SDK (and 13.3) and Apple clang 15.0.0.
NATRON_MACOS_CLT_VERSION=15.3
# tart base image of the SDK image and of the clean test VMs (tools/tart), pinned by digest so that
# a release SDK image is reproducible: the image used on 2026-11-21 (macOS 14.8.7). To try the
# newest one, override it: BASE_IMAGE=ghcr.io/cirruslabs/macos-sonoma-base:latest; to move the
# pin, put the new digest here (tart pull prints it; tart list shows it).
NATRON_MACOS_BASE_IMAGE=ghcr.io/cirruslabs/macos-sonoma-base@sha256:e2ebdfc4d354b336fe00d729c11a8136a019b8046f41cc183a2fe51b85d18f49
