#!/bin/sh

dir=$(dirname $0)

for uml in $dir/*.uml;
do
    echo Compiling $uml
    plantuml -failonerror -config $dir/plantuml.cfg -tsvg $uml -o $(pwd) || exit 1
done

