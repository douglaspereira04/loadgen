#!/bin/bash

#Compare files with test files in this directory with sha1sum 
#Copares the file named <filename> with the file named test_<filename>)

for file in *; do
    if [ -f "$file" ]; then
		if [ -f "test_${file}" ]; then
			#stores sha1sum in variable
			sha1sum_file=$(sha1sum "$file")
			sha1sum_test_file=$(sha1sum "test_${file}")
			if [ "$sha1sum_file" != "$sha1sum_test_file" ]; then
				echo "File $file is different from test_${file}"
			fi
		fi
	fi
done;