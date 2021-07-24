# bash-backup
A headless backup system for websites with databases runnable from cron

## Description

This is a simple backup system written in bash.  It depends on rsync.  You
can think of this program as a program to calculate the options you want
to pass to rsync so that you don't have to figure them out.

There are two scripts.  bb_server.inc is for making a backup of a
remote server, including possible mysql database and git repos.
bb_desktop.inc is for making a backup of a desktop machine to another
computer.

Backups of files are performed using rsync, and will utilize the
--link-dest argument in order to create incremental backups.  This
will cause a new backup folder to contain hardlinks to files that did
not change since the previous backup.

For server backups special provisions are made for backing up mysql
databases and git repos.  (I use git to automate deploys to servers
typically so I want a backup of that, but each to their own.)

## Desktop backups

A script to backup a desktop might look like:

```
#!/bin/bash

. ./bb_desktop.inc

declare -A CONFIG
#uncomment for extreme verbosity
#set -x

CONFIG[SERVER]=1.2.3.4
CONFIG[STAMP]=$(date +%FT%T)
CONFIG[DESTINATION_FOLDER]=/vault/backups/machines/$(hostname)

bb_desktop "$(declare -p CONFIG)"
```

`CONFIG[SERVER]=1.2.3.4` can be used to backup over a network to a
remote machine via ssh.  This is optional however, and a backup to a
locally mounted disk is possible by simply not specifying SERVER in
the CONFIG.

STAMP is required and must be unique per backup.

DESTINATION_FOLDER is also required, and should be the same for all backups
in order for the rsync incremental backup feature to work.

## Server backups

Backups can be performed of remote systems, this will depend on `ssh`.

Also special provisions are made to backup mysql databases using mysqldump,
and also git repos if you want to back them up as well.

## Usage

The way the script works is that you create a bash script that acts as
a configuration file and sources the bb_server.inc script.
Configuration parameters are communicated to bb_server.inc as a single
associate array in BASH.  If you have more than one backup to do, you
can create a simple script to execute all the backup configurations in
order.

Note: Bash barely supports associative arrays, so its a little hacky
especially since we intend to support Bash 4.2.

The backup happens over ssh.  You will need to install the public key
from the account you run the script from onto the server that you
intend to backup.  Note if you don't have access to the SSH on your
server, this isn't the right approach for you to implement backups.

The next requirement is that the SSH account on the side you intend to
backup either has access to the resources it intends to backup, with
the following exception: You can `sudo` without a password on the
remote side to an account that has access instead.

The associate array configuration variables bash backup reads are as follows:

### required

VERBOSITY - optional, set to 'silent' to filter away known messages
that are expected when commands are successful.  Set to '1' for extra
debugging info.  Not setting this will simply output normal messages
from git, rsync, etc. as they run.  Note bash has 'set -x' which
creates extreme verbosity.  You can set that before calling
`bb_backup`.

STAMP - required - used to denote the version number of this backup.

REMOTE_SERVER - required - the remote server in SSH format, should
include usename if necessary. i.e.  `deploy@my.webserver.com`

DESTINATION_FOLDER - required - local folder where you want the backup written.

### for backing up files

REMOTE_FILES - optional - folder that you would like rsycned into the backup.

REMOTE_FILES_USER - optional - if the user specified in the
1REMOTE_SERVER` does not have read access to all files that need to
be backed up, specify a user that does here.

### for backing up git repo

REMOTE_GIT_REPO - optional - if there is a git repo on the
`REMOTE_SERVER` you would like backed up, specify its remote path
here.  Intended use case is for a bare repo which ends in `.git`

### for backing up database

REMOTE_DB_USER - database username for database to back up.

REMOTE_DB_NAME - database name for database to back up.

REMOTE_DB_PASSWORD - database password for database you would like
to back up.  Note you can put in a command here to read the password
on the remote side to keep the password out of this file.  For example
for wordpress:
```
REMOTE_DB_PASSWORD="\$(sudo -u deployAccount -s cat /path/to/wp-config.php | awk '/define.*DB_PASSWORD/ { print \"<?php \"; print $1; print \" echo DB_PASSWORD ?>\"  } ' | php)"

```
That snippet is sudo-ing to an account that can read the password,
reading the file `wp-config.php` which contains the password, finding
the line that defines the password, then passes that code to PHP to
render the password as output.

Another snippet I used on a drupal site:
```
REMOTE_DB_PASSWORD="\$(cat ~/project/config/database.yml | sed -n '/^production:/,\$p' | grep 'password:' | awk '{ print \$2 }')"
```

A typical configuration file might look like:
```
#!/bin/bash

. ./bb_server.inc

declare -A CONFIG
#uncomment for extreme verbosity
#set -x

CONFIG[VERBOSITY]=1
CONFIG[STAMP]=$(date +%FT%T)
CONFIG[SERVER]=me@site.com

CONFIG[DESTINATION_FOLDER]=/where/I/keep/backups/site.com
CONFIG[REMOTE_FILES]=/www/<user>/site.com
CONFIG[REMOTE_FILES_USER]=<user>

CONFIG[REMOTE_DB_USER]=mydbuser
CONFIG[REMOTE_DB_NAME]=mydbname
CONFIG[REMOTE_DB_PASSWORD]="\$(sudo -u <user> -s cat /var//www/site.com/_root_/wp-config.php | awk '/define.*DB_PASSWORD/ { print \"<?php \"; print $1; print \" echo DB_PASSWORD ?>\"  } ' | php)"

bb_server "$(declare -p CONFIG)"
```