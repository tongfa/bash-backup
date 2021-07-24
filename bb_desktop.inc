#!/bin/bash

set -e

BU_SERVER=169.254.88.231
BU_DESTINATION=/vaultBackup2/backups/machines/$(hostname)
BU_VER=$(date +%FT%T)

function initialize {
    if [ "${BB[SERVER]}" ] ; then
	BU_COMMAND_SHELL="ssh ${BB[SERVER]}"
	BU_COMMAND_CP="scp"
	BU_DESTINATION_PREFIX="${BB[SERVER]}:"
    else
	BU_COMMAND_SHELL="bash -c" 
	BU_COMMAND_CP="cp"
	BU_DESTINATION_PREFIX=""
    fi
    echo DEST ${BU_DESTINATION_PREFIX}${BB[DESTINATION_FOLDER]}
    echo VER ${BB[STAMP]}
    ${BU_COMMAND_SHELL} "mkdir -p ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/;\
touch ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/IN_PROGRESS"
}

function backup_files {
    DIR=${1%/}
    if [ "${DIR:0:1}" == "/" ] ; then
        DIR=${DIR:1}
    fi
    ${BU_COMMAND_SHELL} "mkdir -p $(dirname ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/${DIR})"
    rsync -a --link-dest=${BB[DESTINATION_FOLDER]}/latest_rsync/${DIR} ${DIR}/ ${BU_DESTINATION_PREFIX}${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/${DIR}/ || echo "rsync failure" $?
}

function backup_file {
    FILENAME=${1}
    if [ "${FILENAME:0:1}" == "/" ] ; then
        FILENAME=${FILENAME:1}
    fi
    DIRNAME=$(dirname ${FILENAME})
    ${BU_COMMAND_SHELL} "mkdir -p ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/${DIRNAME}"
    ${BU_COMMAND_CP} $FILENAME ${BU_DESTINATION_PREFIX}${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/${DIRNAME}/
}

function finalize {
    ${BU_COMMAND_SHELL} "\
if [ -e ${BB[DESTINATION_FOLDER]}/latest_rsync ] ; then \
    rm ${BB[DESTINATION_FOLDER]}/latest_rsync; \
fi; \
ln -s ${BB[STAMP]} ${BB[DESTINATION_FOLDER]}/latest_rsync; \
rm ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/IN_PROGRESS;\
"
}

function bb_desktop {
    eval "declare -A BB="${1#*=}

    cd /

    #http://www.linuxveda.com/2015/04/28/upgrade-from-debian-7-to-debian-8/
    # /etc/
    # /var/lib/dpkg
    # /var/lib/apt/extended_states
    # /var/lib/aptitude/pkgstates
    # dpkg --get-selections "*"

    initialize
    
    backup_files /etc/
    backup_files /var/lib/dpkg/
    backup_file /var/lib/apt/extended_states
    if [ -e /var/lib/aptitude ] ; then
        backup_files /var/lib/aptitude
        fi
    dpkg --get-selections "*" >> /tmp/dpkg-get-selections
    backup_file /tmp/dpkg-get-selections

    for home_dir in $(ls -1 /home) ; do
        backup_files /home/${home_dir}/
    done

    finalize
}

