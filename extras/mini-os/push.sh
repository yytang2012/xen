#!/bin/bash
set -eux

# Use --delete for a completely clean build
# rsync --exclude .git --delete -ri ~/work/cubieboard/mini-os root@build:

# Just sync the extras/mini-os directory
echo Syncing...
rsync -a ~/work/cubieboard/mini-os/extras/mini-os root@build:mini-os/extras/

ssh root@build sh -c "'cd mini-os/extras/mini-os && make'"
scp root@build:mini-os/extras/mini-os/mini-os.img .
scp mini-os.img root@linaro-developer:
ssh root@linaro-developer 'xl destroy Mini-OS; xl create mini-os.cfg; sleep 2; xl destroy Mini-OS'
