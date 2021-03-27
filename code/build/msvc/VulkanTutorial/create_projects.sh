#!/bin/bash

# after running this script, use
# https://marketplace.visualstudio.com/items?itemName=MaciejGudanowicz.AddMultipleProjectsToSolution
# to add all generated projects to the solution.

set -ex
set -o pipefail

codedir=../../..
srcproject=29_multisampling

projects=`find $codedir -maxdepth 1 -type f -name '*.cpp' -printf "%f\n" | while read l; do echo ${l%.cpp}; done |grep -v ^$srcproject | sort`
curfrag=""
curvert=""

for proj in $projects; do
    projnum=${proj%%_*}
    if frag=$(ls $codedir/${projnum}_*.frag); then
        curfrag=$(basename "$frag")
    fi
    if vert=$(ls $codedir/${projnum}_*.vert); then
        curvert=$(basename "$vert")
    fi
    rm -rf $proj
    cp -ra $srcproject $proj
    for suffix in "" .filters .user; do
        mv $proj/$srcproject.vcxproj$suffix $proj/$proj.vcxproj$suffix
    done
    sed -i $proj/$proj.vcxproj -e "s#29_multisampling.cpp#$proj.cpp#"
    sed -i $proj/$proj.vcxproj.filters -e "s#29_multisampling.cpp#$proj.cpp#"
    if [[ -n "$curvert" ]]; then
        sed -i $proj/$proj.vcxproj -e "s#26_shader_depth.vert#$curvert#"
        sed -i $proj/$proj.vcxproj.filters -e "s#26_shader_depth.vert#$curvert#"
    fi
    if [[ -n "$curfrag" ]]; then
        sed -i $proj/$proj.vcxproj -e "s#26_shader_depth.frag#$curfrag#"
        sed -i $proj/$proj.vcxproj.filters -e "s#26_shader_depth.vert#$curvert#"
    fi
done


