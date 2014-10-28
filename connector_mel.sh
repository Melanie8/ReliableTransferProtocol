#!/usr/bin/env bash

# This script was written by Matt Craig
# This script comes with NO Warranty.  Make sure you read the script

# And understand what it does before you use it. It is Your responsibility
# to ensure this does what you want it to do BEFORE you run it.

# I have attempted to make sure it can't break anything...
# but SHELL SCRIPTS ARE DANGEROUS. Use this at your own peril.
# AND NEVER EVER EVER RUN SCRIPTS LIKE THIS AS ROOT.



# The default user to try to connect as
defaultUser="sedda"

# The remote host to connect to. Can be IP or Host name
defaultRemoteHost="studssh.info.ucl.ac.be"

# The path on the remote host to access '/home/user/'
defaultRemoteDir="/etinfo/users/2013/sedda/Documents/ReliableTransferProtocol"

# The local directory used to MOUNT the remote system on. "/home/user/some_folder"
# NO TRAILING SLASH!!!!
defaultLocalDir="/Users/melaniesedda/Desktop/RTP"


function Usage {
	echo "-------------------------------------------------------------------"
	echo "Usage:"
	echo "$0 [-a action] [-u user] [-r RemoteHost] [-d RemoteDir] [-l LocalDir]"
	echo "	    -a The action to take 'connect' or 'disconnect'. You can manually "
	echo "         specify the action to take.  If you do not specify an action"
	echo "         the script will attempt to 'detect' the action you want by "
	echo "         checking the mount point (Optional)"
	echo "	    -u The user to connect as. (Optional)"
	echo "	    -r The remote host (or IP) to connect to. (Optional)"
	echo "	    -d The remote directory to connect to. (Optional)"
	echo "	    -l The local directory to mount to. (Optional) No Trailing slash."
	echo ""
	echo "Note about Optional Parameters:"
	echo ""
	echo "You MUST configure this script PRIOR to running inorder make"
	echo "the 'optional' parameters optional.  If you have not configured"
	echo "this script, ALL PARAMETERS ARE REQUIRED!"
	echo "-------------------------------------------------------------------"
	echo "Information:"
	echo "This script was written by Matt Craig, information about this"
	echo "can be found at http://taggedzi.com/"
	echo "Copyright 2010 Matthew Craig."
	echo "-------------------------------------------------------------------"
	echo "Disclaimer: "
	echo ""
	echo "This script come with ABSOLUTELY NO WARRANTY WHAT SO EVER."
	echo ""
	echo "This is simply a handy script that I use and have decided so share."
	echo ""
	echo "Make sure you have read and understand what this script is doing"
	echo "BEFORE you use it.  It is YOUR responsibility to make sure"
	echo "it will not hurt, damage, or disrupt your systems. As with any shell"
	echo "script it is completly capable of destroying your entire system. "
	echo "So use this at your own Peril!"
	echo ""
	echo "I have attempted to make sure it can't 'break' anything, "
	echo "but I cannot account for every system, setup, user, or input"
	echo "that can happen."
	echo "-------------------------------------------------------------------"  

}

Usage

# Basic Usage information
if [ "$#" == 0 ]; then
	echo "No parameters specified. Attempting to Detect action needed."
fi

# Get Args
while getopts "a:u:r:d:l:" o
	do
		case "$o" in
			a)action="$OPTARG";;
			u)user="$OPTARG";;
			r)remoteHost="$OPTARG";;
			d)remoteDir="$OPTARG";;
			l)localDir="$OPTARG";;
		esac
	done
shift $((OPTIND-1))

# if no remote host specified use default
if [ "$localDir" == "" ]; then
	# if no default set... display message and exit
	if [ "$defaultLocalDir" == "" ]; then
		echo "-------------------------------------------------------------------"
		echo "Error:"
		echo "You have not specified a local directory and no default is set."
		exit 1
	fi
	localDir="$defaultLocalDir"
fi

# Test the local directory
if [ -d "$localDir" ]; then
	#this is a good directory
	echo ""
else 
	#this is not a directory
	echo "-------------------------------------------------------------------"
	echo "Error:"
	echo "The local directory you specified '$localDir' does not exist."
	exit 1
fi

echo "$localDir"

# Verify An action has been specified
if [ "$action" == "" ]; then
	# test to see if the folder specified is already mounted.
	if grep -qs "$localDir" /proc/mounts; then
		echo "File system is already mounted. I assume you wish to disconnect."
		action="disconnect"
	else
		echo "File system is NOT mounted. I assume you wish to connect."
		action="connect"
	fi
fi

# if connecting these parameters are required.
if [ "$action" == "connect" ]; then

	# if no user specified use default
	if [ "$user" == "" ]; then
		# if no default set... display message
		if [ "$defaultUser" == "" ]; then
			echo "-------------------------------------------------------------------"
			echo "Error:"
			echo "You have not specified a user and no default is set."
			exit 1
		fi
		user="$defaultUser"
	fi

	# if no remote host specified use default
	if [ "$remoteHost" == "" ]; then
		# if no default set... display message and exit
		if [ "$defaultRemoteHost" == "" ]; then
			echo "-------------------------------------------------------------------"
			echo "Error:"
			echo "You have not specified a remote Host and no default is set."
			exit 1
		fi
		remoteHost="$defaultRemoteHost"
	fi

	# if no remote dir specified use default
	if [ "$remoteDir" == "" ]; then
		# if no default set... display message and exit
		if [ "$defaultRemoteDir" == "" ]; then
			echo "-------------------------------------------------------------------"
			echo "Error:"
			echo "You have not specified a remote directory and no default is set."
			exit 1
		fi
		remoteDir="$defaultRemoteDir"
	fi
fi 





# DO the action!
case "$action" in
	connect)
		echo "-------------------------------------------------------------------"
		echo "Results:"
		echo "Attempted to connect to $remoteHost"
		echo "Running Command: sshfs $user@$remoteHost:$remoteDir $localDir"
		echo `sshfs "$user@$remoteHost:$remoteDir" "$localDir"`
	;;
	disconnect)
		echo "-------------------------------------------------------------------"
		echo "Results:"
		echo "Attempted to disconnect from $remoteHost"
		echo "Running Command: fusermount -u $localDir" 
		echo `fusermount -u "$localDir"`

	;;
	*)
		echo "-------------------------------------------------------------------"
		echo "Error:"
		echo "Specified action cannot be taken. Please look at the usage of this script."
		exit 1
esac
