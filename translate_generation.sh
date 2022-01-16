#!/bin/bash
# this file is used to auto-generate .qm file from .ts file.

ts_list=(`ls ./translations/*.ts`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    lrelease "${ts}"
done
