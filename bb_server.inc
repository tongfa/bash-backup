# this file is intended to be sourced from another file containing a configuration to drive it.
# see README.md
set -e

function bb_prepare {
    if [ "${BB[_VERBOSITY]}" -ge "1" ] ; then
       echo "preparing for stamp: ${BB[STAMP]}"
    fi
    if [ ! -d ${BB[DESTINATION_FOLDER]} ] ; then
        mkdir -p ${BB[DESTINATION_FOLDER]}
    fi
    if [ ! -d ${BB[DESTINATION_FOLDER]}/${BB[STAMP]} ] ; then
	mkdir ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}
    fi
    touch ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/IN_PROGRESS
}

function bb_finish {
    if [ "${BB[_VERBOSITY]}" -ge "1" ] ; then
       echo "finish"
    fi
    rm ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/IN_PROGRESS
}

function bb_download_db {
    if [ "${BB[_VERBOSITY]}" -ge "1" ] ; then
       echo "downloading db using: ${BB[_SQL_DUMP_CMD]}"
    fi
    ssh ${BB[SERVER]} ${BB[_SQL_DUMP_CMD]} > ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}/db.sql
}

function bb_download_db_specified_by_wpconfig {
	 BB[_SQL_DUMP_CMD]="mysqldump \
-h \$(cat ${BB[DB_WPCONFIG]} | awk '/define.*DB_HOST/ { print \"<?php \"; print $1; print \" echo DB_HOST ?>\"  } ' | php)    \
-u \$(cat ${BB[DB_WPCONFIG]} | awk '/define.*DB_USER/ { print \"<?php \"; print $1; print \" echo DB_USER ?>\"  } ' | php)    \
\$(cat ${BB[DB_WPCONFIG]} | awk '/define.*DB_NAME/ { print \"<?php \"; print $1; print \" echo DB_NAME ?>\"  } ' | php)       \
-p\$(cat ${BB[DB_WPCONFIG]} | awk '/define.*DB_PASSWORD/ { print \"<?php \"; print $1; print \" echo DB_PASSWORD ?>\"  } ' | php) "
}

function bb_download_db_with_credentials {
    BB[_SQL_DUMP_CMD]="mysqldump ${BB[REMOTE_DB_NAME]}"
    if [ "${BB[REMOTE_DB_USER]}" ] ; then
	BB[_SQL_DUMP_CMD]+=" -u ${BB[REMOTE_DB_USER]}"
    fi
    if [ "${BB[DBHOST]}" ] ; then
	BB[_SQL_DUMP_CMD]+=" -h ${BB[DBHOST]}"
    fi
    if [ "${BB[REMOTE_DB_PASSWORD]}" ] ; then
	BB[_SQL_DUMP_CMD]+=" -p${BB[REMOTE_DB_PASSWORD]}"
    fi
}

function bb_download_git {
    if [ "${BB[_VERBOSITY]}" -ge "1" ] ; then
       echo "downloading git repo ${BB[REMOTE_GIT_REPO]}"
    fi
    BB[_GITDEST]=${BB[DESTINATION_FOLDER]}/$(basename $(echo ${BB[REMOTE_GIT_REPO]} | sed -e 's/\.git$//')).git
    if [ ! -e "${BB[_GITDEST]}" ] ; then
	git clone --mirror ${BB[SERVER]}:${BB[REMOTE_GIT_REPO]} ${BB[_GITDEST]}
    else
	(cd  ${BB[_GITDEST]}; git remote update)
    fi
}

function bb_download_files {
    if [ "${BB[_VERBOSITY]}" -ge "1" ] ; then
       echo "downloading files from: ${BB[REMOTE_FILES]}"
    fi
    if [ "${BB[REMOTE_FILES_USER]}" ] ; then
       RSYNCPATH="sudo -u ${BB[REMOTE_FILES_USER]} -s rsync"
    else
       RSYNCPATH=rsync
    fi
    rsync --rsync-path="${RSYNCPATH}" -az --link-dest=../latest_rsync ${BB[SERVER]}:${BB[REMOTE_FILES]} ${BB[DESTINATION_FOLDER]}/${BB[STAMP]}
    if [ -e ${BB[DESTINATION_FOLDER]}/latest_rsync ] ; then
	rm ${BB[DESTINATION_FOLDER]}/latest_rsync
    fi
    ln -s ${BB[STAMP]} ${BB[DESTINATION_FOLDER]}/latest_rsync
}

function bb_test_db_specified_by_wpconfig {
    if [ "${BB[DB_WPCONFIG]}" ] ; then
	# this will populate BB_SQL_DUMP_CMD
	bb_download_db_specified_by_wpconfig
    fi	
    echo dump command "${BB[_SQL_DUMP_CMD]}"
    echo dump command on server:
    ssh ${BB[SERVER]} echo "${BB[_SQL_DUMP_CMD]}"
}

function bb_test_db {
    if [ "${BB[REMOTE_DB_NAME]}" ] ; then
	# this will populate BB_SQL_DUMP_CMD
	bb_download_db_with_credentials
    fi	
    echo dump command "${BB[_SQL_DUMP_CMD]}"
    echo dump command on server:
    ssh ${BB[SERVER]} echo "${BB[_SQL_DUMP_CMD]}"
}

function bb_squelch {
    grep -v 'Fetching origin'
}

function bb_server {
    eval "declare -A BB="${1#*=}
    if [ "${BB[VERBOSITY]}" == "silent" ] ; then
	BB_OUTPUT_FILTER=bb_squelch
	BB[_VERBOSITY]=0
    else
	BB[_VERBOSITY]="${BB[VERBOSITY]-0}"
        BB_OUTPUT_FILTER=cat
    fi
    bb_prepare $1
    if [ "${BB[DB_WPCONFIG]}" ] ; then
	# this will populate _SQL_DUMP_CMD
	bb_download_db_specified_by_wpconfig
    fi	
    if [ "${BB[REMOTE_DB_NAME]}" ] ; then
	# this will populate _SQL_DUMP_CMD
	bb_download_db_with_credentials
    fi
    # one of the above should had populated BB[_SQL_DUMP_CMD] already.
    if [ "${BB[_SQL_DUMP_CMD]}" != "" ] ; then
       bb_download_db
    fi
    if [ "${BB[REMOTE_GIT_REPO]}" != "" ] ; then
	bb_download_git | ${BB_OUTPUT_FILTER}
    fi
    if [ "${BB[REMOTE_FILES]}" != "" ] ; then
	bb_download_files
    fi
    bb_finish
}
