# this file is sourced by the build-plugins.sh script if it exists
IO_BRANCH=master
MISC_BRANCH=master
ARENA_BRANCH=master
GMIC_BRANCH=master
CV_BRANCH=master

# Pin a commit as well as a branch. Tracking master means an upstream plugin
# change alters what this Natron builds with no commit landing here, which
# makes a red build ambiguous: it could be this tree or it could be somebody
# else's. Bump these deliberately.
#
# The shas below are the ones the Windows installer build was last verified
# against. Full 40 characters: a shallow fetch rejects an abbreviated sha with
# "not our ref".
IO_COMMIT=1bce1f1a2ad05c793682dd6845e62e5b352a3ea6
MISC_COMMIT=0abd46b5a8cbc98fa24579042129460d0aa87b8f
ARENA_COMMIT=49e4d5fc197a637196c1a7decab7f97751b867f6
GMIC_COMMIT=292ac55c7c84f6eff56c1764a5a5037b96895d12
CV_COMMIT=
