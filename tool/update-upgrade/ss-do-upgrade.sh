#!/bin/bash

echo "以上可升级，是否升级？[y/n]"
read yes_or_no
if [ "$yes_or_no" = "y" ];then
mkdir -p /tmp/ss-apt-fast-conf/sources.list.d


sudo bwrap --dev-bind / / --bind '/opt/durapps/spark-store/bin/apt-fast-conf/sources.list.d/sparkstore.list' /etc/apt/sources.list.d/sparkstore.list ss-apt-fast upgrade -y -o Dir::Etc::sourcelist="sources.list.d/sparkstore.list"     -o Dir::Etc::sourceparts="-" -o APT::Get::List-Cleanup="0"


else
exit
fi