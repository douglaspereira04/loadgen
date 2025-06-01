#!/bin/bash

for file in workloads/*; do
	filename=$(basename -- "$file")
	filename="${filename%.*}"
	requests_file_name=${filename%.*}_requests.txt
	if ls ${v,,}* 1> /dev/null 2>&1; then
		if [ ! -f ${requests_file_name} ]; then
			./gen ${file}
			mv requests.txt ${requests_file_name}
		fi
	fi
done;
