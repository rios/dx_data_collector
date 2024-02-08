#!/bin/bash
set -e

CURRENT_DATETIME=$(date +"%s")
CURRENT_REPO=$(basename `git rev-parse --show-toplevel`)
CURRENT_BRANCH=`git rev-parse --abbrev-ref HEAD | sed 's/\//_/g'`
if [ "$CURRENT_REPO" = "<internal_build_framework>" ]
then
    CURRENT_BRANCH_IN_URL=:`git rev-parse --abbrev-ref HEAD`
else
    CURRENT_BRANCH_IN_URL=""
fi
CURRENT_COMMIT_HASH=`git log --pretty=format:"%h" -1`

earthly ${@} --build-arg CURRENT_DATETIME=$CURRENT_DATETIME --build-arg CURRENT_BRANCH=$CURRENT_BRANCH --build-arg CURRENT_COMMIT_HASH=$CURRENT_COMMIT_HASH --build-arg CURRENT_REPO=$CURRENT_REPO --build-arg CURRENT_BRANCH_IN_URL=$CURRENT_BRANCH_IN_URL +tag-image

docker image ls --filter "label=buildtime=$CURRENT_DATETIME" --format "{{.Repository}}:{{.Tag}}" | grep -v '_linux_' | xargs
