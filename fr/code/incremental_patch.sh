#!/bin/sh

# Check if both a starting file and patch are provided
if [ $# != 2 ]; then
    echo "usage: <first_file.cpp> <patch.txt>"
    echo "specified patch will be applied to first_file.cpp and every code file larger than it (from later chapters)"
    exit 1
fi

# Iterate over code files in order of increasing size
# i.e. in order of chapters (every chapter adds code)
apply_patch=false

for f in `ls -Sr *.cpp`
do
    # Apply patch on every code file including and after initial one
    if [ $f = $1 ] || [ $apply_patch = true ]; then
        apply_patch=true

        patch -f $f < $2 | grep -q "FAILED" > /dev/null
        if [ $? = 0 ]; then
            echo "failed to apply patch to $f"
            exit 1
        fi

        rm -f *.orig
    fi
done

echo "patch successfully applied to all files"
exit 0
