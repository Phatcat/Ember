#!/bin/bash

# Copyright (c) 2021 Ember
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

CONFIG_FILE="run_win.config"

echo "Welcome to the Ember server install/update script for Windows."
echo

function first_run {
echo "You seem to be missing a config file for this script."
echo "We will create one for you in an instant"
echo
echo "Please enter the complete path to your ember source files directory"
read -p 'ember source directory: ' EMBER_SOURCE_PATH
echo
echo "Please enter the complete path to your ember install directory"
read -p 'ember install directory: ' EMBER_INSTALL_PATH
echo
echo "Please enter the complete path to the tools deployment directory"
read -p 'ember tools deployment directory: ' EMBER_TOOL_PATH
echo
echo "Please enter a name for the ember login database"
read -p 'ember login database name: ' EMBER_LOGIN_NAME
echo
echo "Please enter a name for the ember world database"
read -p 'ember world database name: ' EMBER_WORLD_NAME
echo
echo "Please enter a name for the ember login database user"
read -p 'ember login database username: ' EMBER_LOGIN_USER
echo
echo "Please enter a password for the ember login database user"
read -p 'ember login database user password: ' EMBER_LOGIN_PASS
echo
echo "Please enter a name for the ember world database user"
read -p 'ember world database username: ' EMBER_WORLD_USER
echo
echo "Please enter a password for the ember world database user"
read -p 'ember world database user password: ' EMBER_WORLD_PASS
echo

cat >  $CONFIG_FILE << EOF
# The path to the source directory of ember
EMBER_SOURCE_PATH=$EMBER_SOURCE_PATH

# The path to the install directory of ember
EMBER_INSTALL_PATH=$EMBER_INSTALL_PATH

# The path to the tools deployment folder
EMBER_TOOL_PATH=$EMBER_TOOL_PATH

# The adress of the server host
SERVER_HOST=localhost

# the ember login database name
EMBER_LOGIN_NAME=$EMBER_LOGIN_NAME

# The ember world database name
EMBER_WORLD_NAME=$EMBER_WORLD_NAME

# The ember login database username
EMBER_LOGIN_USER=$EMBER_LOGIN_USER

# The ember login database password
EMBER_LOGIN_PASS=$EMBER_LOGIN_PASS

# The ember world database username
EMBER_WORLD_USER=$EMBER_WORLD_USER

# The ember world database password
EMBER_WORLD_PASS=$EMBER_WORLD_PASS
EOF
}

if [ ! -f $CONFIG_FILE ]
then
	first_run
fi

. $CONFIG_FILE
export EMBER_PATH="$EMBER_SOURCE_PATH"
       INSTALL_PATH="$EMBER_INSTALL_PATH"
       TOOL_PATH="$EMBER_TOOL_PATH"
       HOST_ADDRESS="$SERVER_HOST"
       EMBER_LOGIN_DB="$EMBER_LOGIN_NAME"
       EMBER_WORLD_DB="$EMBER_WORLD_NAME"
       EMBER_LOGIN_DB_USER="$EMBER_LOGIN_USER"
       EMBER_LOGIN_DB_PASS="$EMBER_LOGIN_PASS"
       EMBER_WORLD_DB_USER="$EMBER_WORLD_USER"
       WORLD_USER_DB_PASS="$EMBER_WORLD_PASS"

echo "Please enter your mysql root credentials"
read -p 'mysql root username: ' MYSQL_ROOT_USER
read -sp 'mysql root pass: ' MYSQL_ROOT_PASS
echo

echo "You now have the following options;"
echo "If you want to set up the ember servers, press 'i' for install."
echo "If you want to purge the ember servers, press 'c' for clean."
echo "If you want to update the ember servers, press 'u' for update"
read -n1 USER_ACTION
echo "$USER_ACTION"
echo

cd $TOOL_PATH
DBCPAR=$(realpath ./dbc-parser)
DBUTIL=$(realpath ./dbutils)

if [[ $USER_ACTION == *"i"* ]]
then
	echo "Performing initial database installation and updates..."
	$DBUTIL --install login world --update login world --sql-dir ${EMBER_PATH}/sql/ \
	        --login.root-user $MYSQL_ROOT_USER --login.root-password $MYSQL_ROOT_PASS \
	        --login.hostname $HOST_ADDRESS --login.set-user $EMBER_LOGIN_DB_USER --login.set-password $EMBER_LOGIN_DB_PASS \
	        --login.db-name $EMBER_LOGIN_DB \
	        --world.root-user $MYSQL_ROOT_USER --world.root-password $MYSQL_ROOT_PASS \
	        --world.hostname $HOST_ADDRESS --world.set-user $EMBER_WORLD_DB_USER --world.set-password $WORLD_USER_DB_PASS \
	        --world.db-name $EMBER_WORLD_DB
elif [[ $USER_ACTION == *"c"* ]]
then
	echo "Performing a clean database installation and updates..."
	$DBUTIL --install login world --clean --update login world --sql-dir ${EMBER_PATH}/sql/ \
	        --login.root-user $MYSQL_ROOT_USER --login.root-password $MYSQL_ROOT_PASS \
	        --login.hostname $HOST_ADDRESS --login.set-user $EMBER_LOGIN_DB_USER --login.set-password $EMBER_LOGIN_DB_PASS \
	        --login.db-name $EMBER_LOGIN_DB \
	        --world.root-user $MYSQL_ROOT_USER --world.root-password $MYSQL_ROOT_PASS \
	        --world.hostname $HOST_ADDRESS --world.set-user $EMBER_WORLD_DB_USER --world.set-password $WORLD_USER_DB_PASS \
	        --world.db-name $EMBER_WORLD_DB
elif [[ $USER_ACTION == *"u"* ]]
then
	echo "Performing database updates..."
	$DBUTIL --update login world --sql-dir ${EMBER_PATH}/sql/ \
	        --login.root-user $MYSQL_ROOT_USER --login.root-password $MYSQL_ROOT_PASS \
	        --login.hostname $HOST_ADDRESS --login.set-user $EMBER_LOGIN_DB_USER --login.set-password $EMBER_LOGIN_DB_PASS \
	        --login.db-name $EMBER_LOGIN_DB \
	        --world.root-user $MYSQL_ROOT_USER --world.root-password $MYSQL_ROOT_PASS \
	        --world.hostname $HOST_ADDRESS --world.set-user $EMBER_WORLD_DB_USER --world.set-password $WORLD_USER_DB_PASS \
	        --world.db-name $EMBER_WORLD_DB
else
	echo "Faulty input: $USER_ACTION is not an option!"
	exit 1
fi

if ! [ $? -eq 0 ]
then
	exit 1
fi

echo "Generating default DBCs..."
$DBCPAR -o ${INSTALL_PATH}/dbcs/ -d ${EMBER_PATH}/dbcs/definitions/client/ ${EMBER_PATH}/dbcs/definitions/server/ --dbc-gen