# shellcheck shell=bash
# Source this file to build with MacPorts, never Homebrew, in the current shell: PATH has only
# MacPorts (/opt/local) and the system directories, and no HOMEBREW_* variable is set (a Homebrew
# tool on PATH silently pulls in Homebrew headers and libraries).
#
#   . tools/MacPorts/macports-env.sh
export PATH=/opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin
for _natron_v in $(env | sed -n 's/^\(HOMEBREW_[A-Za-z_]*\)=.*/\1/p'); do unset "$_natron_v"; done
unset _natron_v
