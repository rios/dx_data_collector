#!/bin/bash
#
# Note: No changes should be needed to this file
#  
# This script will be invoked by 'initializeCommand' in .devcontainer/devcontainer.json 
#
# Example usage (in .devcontainer/devcontainer.json):
#
#    To run 'default' image:
#   ./run.sh --image default
#
#    To run image with 'latest' tag:
#   ./run.sh --image latest
#
#    To run any other image, specify it explicitly:
#   ./run.sh --image <internal_registry>/current_repo:current_branch-current_commit_hash
# 
# Note that to run default image, you will need to have built it first without making any git changes afterward

set -e

TEXT_RED='\033[0;31m'
TEXT_YELLOW='\033[0;33m'
TEXT_GREEN='\033[0;33m'
TEXT_NO_COLOR='\033[0m'

CURRENT_DATETIME=$(date +"%s")
CURRENT_REPO=$(basename `git rev-parse --show-toplevel` | tr '[:upper:]' '[:lower:]')
CURRENT_BRANCH=`git rev-parse --abbrev-ref HEAD | sed 's/\//_/g'`
CURRENT_COMMIT_HASH=`git log --pretty=format:"%h" -1`

# Set default image and latest image
CURRENT_DEVELOPMENT_IMAGE=<internal_registry>/$CURRENT_REPO:$CURRENT_BRANCH-$CURRENT_COMMIT_HASH
LATEST_DEVELOPMENT_IMAGE=<internal_registry>/$CURRENT_REPO:latest
DEVELOPMENT_IMAGE=""
EARTHLY_TARGET="+copy-build"

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --image)
            DEVELOPMENT_IMAGE="$2"
            shift
            ;;
        *)
            echo "Passing args: $1"
            ;;
    esac
    shift
done

# If no image is specified, use the develop image
if [[ -z "$DEVELOPMENT_IMAGE" ]]; then
    DEVELOPMENT_IMAGE="develop"
    echo "Docker image is not defined. Using develop image."
fi

# If the image is not a custom image, set it to the appropriate image
case "$DEVELOPMENT_IMAGE" in
    "develop")
        DEVELOPMENT_IMAGE="<internal_registry>/$CURRENT_REPO:develop"
        ;;
    "branch")
        DEVELOPMENT_IMAGE="<internal_registry>/$CURRENT_REPO:$CURRENT_BRANCH"
        ;;
    "current")
        DEVELOPMENT_IMAGE="$CURRENT_DEVELOPMENT_IMAGE"
        ;;
    "latest")
        DEVELOPMENT_IMAGE="<internal_registry>/$CURRENT_REPO:latest"
        ;;
    "main")
        DEVELOPMENT_IMAGE="<internal_registry>/$CURRENT_REPO:main"
        ;;
    "build")
        DEVELOPMENT_IMAGE="$EARTHLY_TARGET"
        CURRENT_DEVELOPMENT_IMAGE="$EARTHLY_TARGET"
        ;;
    *)
        # Custom image, no changes needed
        ;;
esac

echo -e "\nRunning $DEVELOPMENT_IMAGE (HEAD: $CURRENT_REPO:$CURRENT_BRANCH-$CURRENT_COMMIT_HASH)\n"

earthly ${@} \
    --build-arg DEVELOPMENT_IMAGE=$DEVELOPMENT_IMAGE \
    --build-arg CURRENT_DATETIME=$CURRENT_DATETIME \
    --build-arg CURRENT_BRANCH=$CURRENT_BRANCH \
    --build-arg CURRENT_COMMIT_HASH=$CURRENT_COMMIT_HASH \
    --build-arg CURRENT_REPO=$CURRENT_REPO \
    --build-arg USERNAME=$USER \
    --build-arg DEV_ENV=true \
    +tag-image

docker image ls --filter "label=buildtime=$CURRENT_DATETIME" --format "{{.Repository}}:{{.Tag}}" | grep -v '_linux_' | xargs


# Check if the image is up to date and warn the user if not
if [[ "$CURRENT_DEVELOPMENT_IMAGE" != "$DEVELOPMENT_IMAGE" ]]; then
    printf "\n\n"
    echo -e "${TEXT_YELLOW}WARNING:${TEXT_NO_COLOR} $DEVELOPMENT_IMAGE and HEAD ($CURRENT_DEVELOPMENT_IMAGE) are not the same."
    echo -e "${TEXT_YELLOW}WARNING:${TEXT_NO_COLOR} Your local code may have diverged from the image you are running."
    printf "\n\n"
fi