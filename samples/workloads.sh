#!/bin/bash

num_threads=1
if [ $# -eq 1 ]; then
	num_threads=$1
fi

for file in workloads/*; do
	filename=$(basename -- "$file")
	filename="${filename%.*}"
	requests_file_name=${filename%.*}_requests.txt
	if ls ${v,,}* 1> /dev/null 2>&1; then
		if [ ! -f ${requests_file_name} ]; then
			./gen ${file} $num_threads
			mv requests.txt ${requests_file_name}
		fi
	fi
done;
