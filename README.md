# bash-backup
A headless backup system for websites with databases runnable from cron

## Description

This is a simple backup system written in bash with the following dependencies:
1. ssh
2. rsync on both sides (optional)
3. mysqldump on the remote side (optional)
4. git on both sides (optional)

Optional dependencies are optional in the sense that you can choose
not to back up the items the dependency is responsible for.  If you
want to backup a filesystem, you will need item 2.  If you want to
backup a database, you will need item 3.

## Usage

Create a bash script that acts as a configuration file and sources the
bb_backup.inc script.  Configuration parameters are communicated to
bb_backup.inc as a single associative array in BASH.  If you have more
than one backup to do, create a higher level script to execute all
the backup configurations in order.

Note: Bash barely supports associative arrays, so its a little hacky
especially since we intend to support Bash 4.2.

The backup happens over ssh.  You will need to install the public key
from the account you run the script from onto the server that you
intend to backup.  Note if you don't have access to the SSH on your
server, this isn't the right approach for you to implement backups.

The next requirement is that the SSH account on the side you intend to
backup either has access to the resources it intends to backup, with
the following exception: You can `sudo` without a password on the
remote side to an account that has access instead, that is if the
remote account supports `sudo`ing without a password.

The associative array configuration variables bash backup reads are as follows:

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
`REMOTE_SERVER` does not have read access to all files that need to
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

. ./bb_backup.inc

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

bb_backup "$(declare -p CONFIG)"
```