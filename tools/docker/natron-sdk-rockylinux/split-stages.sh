#!/bin/sh
# Split the SDK Dockerfile made by build.sh (PREPARE_ONLY=1) into stages that can be built one
# after the other, in separate jobs (a GitHub-hosted job runs at most 6 hours, and the whole SDK
# takes longer on a 4-CPU runner).
#
# Usage: split-stages.sh DOCKERFILE LAST_STEP...
#
# Each LAST_STEP is the name of a build step (tools/jenkins/include/scripts/pkg/<step>.sh) that
# ends a "RUN ... LAST_STEP=<step> ..." line of DOCKERFILE: the stage ends after that line. The
# steps after the last LAST_STEP make the last stage. It writes, next to DOCKERFILE:
#   Dockerfile.stage1        FROM the Rocky Linux base image: the system packages and the first steps
#   Dockerfile.stageN (N>1)  FROM the image of stage N-1, given as --build-arg BASE=<image>
#   Dockerfile.final         the SDK image: --build-arg INTERMEDIATE=<image of the last stage>
# Every stage is a valid build of the same Dockerfile lines, in the same order, so the result is
# the image that "docker build" of DOCKERFILE makes.
set -eu
[ $# -ge 2 ] || { sed -n 2,15p "$0"; exit 1; }
in="$1"
shift
dir="$(dirname "$in")"
awk -v dir="$dir" -v ends="$*" '
BEGIN {
    n = split(ends, e, " ")
    for (i = 1; i <= n; i++) isend[e[i]] = i
    stage = 1
    out = dir "/Dockerfile.stage1"
    printf "" > out
}
# the second FROM starts the final image
/^FROM / && ++froms == 2 {
    final = dir "/Dockerfile.final"
    print "ARG INTERMEDIATE" > final
    print "FROM ${INTERMEDIATE} AS intermediate" > final
    infinal = 1
}
infinal { print > final; next }
# ARG lines of the first stage are scoped to it: repeat them in the other stages
stage == 1 && /^ARG / { args = args $0 "\n" }
{ print > out }
/^RUN / && match($0, /LAST_STEP=[^ ]+/) {
    step = substr($0, RSTART + 10, RLENGTH - 10)
    if (step in isend) {
        found[step] = 1
        stage++
        close(out)
        out = dir "/Dockerfile.stage" stage
        print "ARG BASE" > out
        print "FROM ${BASE}" > out
        printf "%s", args > out
    }
}
END {
    for (s in isend) if (!(s in found)) { print "Error: no RUN line with LAST_STEP=" s > "/dev/stderr"; exit 1 }
    if (!infinal) { print "Error: no final image (second FROM) in the Dockerfile" > "/dev/stderr"; exit 1 }
    print stage
}' "$in"
