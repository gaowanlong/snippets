#!/bin/bash
# usage: mem_limit.sh <memory size with unit>
# example: ./mem_limit.sh 200G
# Create a separate memory cgroup to limit the memory usage of mongodb

mkdir -p /cgroup_mem
mount -t cgroup -o memory memcg /cgroup_mem
mkdir /cgroup_mem/MongoLimitGroup
echo $1 > /cgroup_mem/MongoLimitGroup/memory.limit_in_bytes
for pid in $(pidof mongod)
do
	echo $pid > /cgroup_mem/MongoLimitGroup/tasks
done
