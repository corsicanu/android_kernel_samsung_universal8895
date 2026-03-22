#!/bin/bash

MAJOR=$(echo $1 | cut -d '.' -f 1)
MAJOR=$((MAJOR+103))
printf "%b" "$(printf '\%03o' $MAJOR)"
