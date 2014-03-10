#!/bin/bash

#   CloudPassage Halo Daemon
#   Unattended installation script for Vendor Integrations
#   -------------------------------------------------------------------------
#   This script is intended to be used for an unattended installation
#   of the CloudPassage Halo daemon.
#
#   IMPORTANT NOTES
#
#     * This script may require adjustment to conform to your server's
#       configuration. Please review this script and test it on a server
#       before using it to install the Halo daemon on multiple servers.
#
#     * This script contains the CloudPassage Halo Daemon Registration Key owned by
#       Vendor Integrations. Keep this script safe - handle it as
#       you would the password to your CloudPassage portal account!
#

# add CloudPassage repository
echo -e '[cloudpassage]\nname=CloudPassage\nbaseurl=http://packages.cloudpassage.com/redhat/$basearch\ngpgcheck=1' | tee /etc/yum.repos.d/cloudpassage.repo > /dev/null

# import CloudPassage public key
rpm --import http://packages.cloudpassage.com/cloudpassage.packages.key

# update yum repositories
yum check-update > /dev/null

# install the daemon
yum -y install cphalo

echo "Key=$1" ; echo "Tag=$2"

# start the daemon for the first time
/etc/init.d/cphalod start --daemon-key=$1 --tag=$2
