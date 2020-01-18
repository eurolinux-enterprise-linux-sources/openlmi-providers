#!/bin/bash
#
# Copyright (C) 2012-2014 Red Hat, Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
# Authors: Roman Rakus <rrakus@redhat.com>
#

schemasurl='http://schemas.dmtf.org/wbem/cim-html/2.34.0/'

re_anyclass="^class[[:blank:]]+([[:alnum:]_]+)[[:blank:]]*:[[:blank:]]*([[:alnum:]_]+)"
re_property="^[[:blank:]]*([[:alnum:]_]*)[[:blank:]]*([]^[:alnum:]_\[]*)[[:blank:]]*;"
re_reference="^[[:blank:]]*([[:alnum:]_]*)[[:blank:]]*REF[[:blank:]]*([]^[:alnum:]_\[]*)[[:blank:]]*;"
re_endclass="^[[:blank:]]*}[[:blank:]]*;[[:blank:]]*"

re_method_start="^[[:blank:]]*([[:alnum:]_]+)[[:blank:]]+([[:alnum:]_]+)\("
re_method_property="^[[:blank:]]*([[:alnum:]_]+)[[:blank:]]+([]^[:alnum:]_\[]+)[[:blank:]]*,"
re_method_property_end="^[[:blank:]]*([[:alnum:]_]+)[[:blank:]]+([]^[:alnum:]_\[]+)[[:blank:]]*);"
re_method_reference='^[[:blank:]]*([[:alnum:]_]+)[[:blank:]]+REF[[:blank:]]+([]^[:alnum:]_\[]+)[[:blank:]]*,'
re_method_reference_end='^[[:blank:]]*([[:alnum:]_]+)[[:blank:]]+REF[[:blank:]]+([]^[:alnum:]_\[]+)[[:blank:]]*);'
re_method_end="[[:blank:]]*)[[:blank:]]*;[[:blank:]]*"
re_par_in_required='[[:blank:]]*\[[[:blank:]]*Required[[:blank:]]*,[[:blank:]]*IN'
re_par_out='[[:blank:]]*\[[[:blank:]]*.*OUT'
# assume if the parameter is not OUT then it's IN, but rather check if the word IN is present
re_par_in='[[:blank:]]*\[[[:blank:]]*.*IN'

usage()
{
  printf "$0 usage: "
  printf "COMMAND [PARAMETER] FILE [SUBDIR]\n"
  printf "Converts mof FILE to wiki type documentation\n"
  printf "Optionaly you can specify wiki SUBDIR"
  printf "Possible COMMANDs:\n"
  printf "all: creates list, properties and methods for all classes\n"
  printf "list: convert only all class names to pretty wiki table\n"
  printf "properties: need PARAMETER class name; converts specified class properties\n"
  printf "methods: need PARAMETER class name; converts specified class methods\n"
  printf "Example: $0 list LMI_Account.mof account\n"
  printf "Example: $0 properties LMI_Account LMI_Account.mof account\n"
}

die_usage()
{
  printf "%s\n" "$@"
  usage;
  exit 1
} >&2

die()
{
  printf "%s\n" "$@"
  exit 1
} >&2


all()
{
  moffile="$1"
  if [[ $2 && $2 =~ [^/]$ ]]; then
    subdir="$2"/
  else
    subdir=$2
  fi
  classes=()
  printf "||= Defined class name =||= Inherited from =||\n"
  while IFS= read line; do
    if [[ $line  =~ $re_anyclass ]]; then
      classes+=(${BASH_REMATCH[1]})
      newclass=${BASH_REMATCH[1]}
      derived=${BASH_REMATCH[2]}
      printf "|| [wiki:$subdir%s %s] || [$schemasurl%s.html %s] ||\n" "$newclass" "$newclass" "$derived" "$derived"
    fi
  done < "$moffile"
  echo
  for i in "${classes[@]}"; do
    props "$i" "$moffile" $subdir
    methods "$i" "$moffile" $subdir
    echo
  done
}

list()
{
  moffile="$1"
  if [[ $2 && $2 =~ [^/]$ ]]; then
    subdir="$2"/
  else
    subdir=$2
  fi
  printf "||= Defined class name =||= Inherited from =||\n"
  while IFS= read line; do
    if [[ $line  =~ $re_anyclass ]]; then
      newclass=${BASH_REMATCH[1]}
      derived=${BASH_REMATCH[2]}
      printf "|| [wiki:$subdir%s %s] || [$schemasurl%s.html %s] ||\n" "$newclass" "$newclass" "$derived" "$derived"
    fi
  done < "$moffile"
}

props()
{
  classname="$1"
  moffile="$2"
  if [[ $3 && $3 =~ [^/]$ ]]; then
    subdir="$3"/
  else
    subdir=$3
  fi

  unset properties references
  declare -A properties
  declare -A references
  declare -i found=0

  while IFS= read line; do
    if (( ! $found )); then
      if [[ $line  =~ ^class[[:blank:]]+($classname)[[:blank:]]*":"[[:blank:]]*([[:alnum:]_]+) ]]; then
        newclass=${BASH_REMATCH[1]}
        derived=${BASH_REMATCH[2]}
        found=1
       fi
    else
      # we have found the class, now show new defined properties
      if [[ $line =~ $re_property ]]; then
        ptype=${BASH_REMATCH[1]}
        pname=${BASH_REMATCH[2]}
        properties["$pname"]="$ptype"
      elif [[ $line =~ $re_reference ]]; then
        ptype=${BASH_REMATCH[1]}
        pname=${BASH_REMATCH[2]}
        references["$pname"]="$ptype"
      elif [[ $line =~ $re_endclass ]]; then
          break;
      fi
    fi
  done < "$moffile"

  if (( $found )); then
    printf "= %s =\n" "$classname"
    printf "Along with inherited properties from [$schemasurl%s.html %s], %s defines also following properties\n" "$derived" "$derived" "$classname"
    printf "||= Property type =||= Property name =||\n"
    for prop in "${!properties[@]}"; do
      printf "|| {{{%s}}} || {{{%s}}}\n" "${properties[$prop]}" "$prop"
    done
    printf "Following references\n"
    printf "||= Class name =||= Reference name =||\n"
    for ref in "${!references[@]}"; do
      cname="${references[$ref]}"
      if [[ "$cname" =~ ^CIM ]]; then
        printf "|| [$schemasurl%s.html %s] || {{{%s}}}\n" "$cname" "$cname" "$ref"
      else
        printf "|| [wiki:$subdir%s %s] || {{{%s}}}\n" "$cname" "$cname" "$ref"
      fi
    done
  else
    printf "Class %s not found in the file %s\n" >&2 "$classname" "$moffile"
    exit 1
  fi
}

methods()
{
  classname="$1"
  moffile="$2"
  if [[ $3 && $3 =~ [^/]$ ]]; then
    subdir="$3"/
  else
    subdir=$3
  fi

  unset found in_method
  declare -i found=0
  declare -i in_method=0

  printf "Following methods\n"
  while IFS= read line; do
    if (( ! $found )); then
      if [[ $line  =~ ^class[[:blank:]]*($classname)[[:blank:]]*":"[[:blank:]]*([[:alnum:]_]*) ]]; then
        newclass=${BASH_REMATCH[1]}
        derived=${BASH_REMATCH[2]}
        found=1
       fi
    else
      if ((! $in_method )); then
        # we have found the class, now show new defined methods
        if [[ $line =~ $re_method_start ]]; then
          printf "== {{{%s}}} ==\n" "${BASH_REMATCH[2]}"
          printf "||= Type =||= Data Type =||= Name =||\n"
          printf "|| return || {{{%s}}} || ||\n" "${BASH_REMATCH[1]}"
          in_method=1
        elif [[ $line =~ $re_endclass ]]; then
            break;
        fi
      else
        if [[ $line =~ $re_par_in_required ]]; then
          printf "|| {{{IN required}}} || "
        elif [[ $line =~ $re_par_out ]]; then
          printf "|| OUT || "
        elif [[ $line =~ $re_par_in ]]; then
          printf "|| IN || "
        elif [[ $line =~ $re_method_property ]]; then
          printf "{{{%s}}} ||" "${BASH_REMATCH[1]}"
          printf "{{{%s}}} ||\n" "${BASH_REMATCH[2]}"
        elif [[ $line =~ $re_method_property_end ]]; then
          printf "{{{%s}}} ||" "${BASH_REMATCH[1]}"
          printf "{{{%s}}} ||\n" "${BASH_REMATCH[2]}"
          in_method=0
        elif [[ $line =~ $re_method_reference ]]; then
          cname="${BASH_REMATCH[1]}"
          rname="${BASH_REMATCH[2]}"
          if [[ "$cname" =~ ^CIM ]]; then
            printf "[$schemasurl%s.html %s] || " "$cname" "$cname"
          else
            printf "[wiki:$subdir%s %s] || " "$cname" "$cname"
          fi
          printf "{{{%s}}} ||\n" "$rname"
        elif [[ $line =~ $re_method_reference_end ]]; then
          cname="${BASH_REMATCH[1]}"
          rname="${BASH_REMATCH[2]}"
          if [[ "$cname" =~ ^CIM ]]; then
            printf "[$schemasurl%s.html %s] || " "$cname" "$cname"
          else
            printf "[wiki:$subdir%s %s] || " "$cname" "$cname"
          fi
          printf "{{{%s}}} ||\n" "$rname"
          in_method=0
        elif [[ $line =~ $re_method_end ]]; then
          in_method=0

        fi
      fi
    fi
  done < "$moffile"

  if (( $found )); then
    :
  else
    printf "Class %s not found in the file %s\n" >&2 "$classname" "$moffile"
    exit 1
  fi

}

export LC_ALL=C
case "$1" in
  all)
    if [[ ! -r "$2" ]]; then
      die "File $2 is not readable"
    fi
    all "$2" $3;;

  list)
    if [[ ! -r "$2" ]]; then
      die "File $2 is not readable"
    fi
    list "$2" $3;;

  properties)
    if [[ ! "$2" ]]; then
      die "You must specify class name"
    fi
    if [[ ! -r "$3" ]]; then
      die "File $3 is not readable"
    fi
    props "$2" "$3" $4;;

  methods)
    if [[ ! "$2" ]]; then
      die "You must specify class name"
    fi
    if [[ ! -r "$3" ]]; then
      die "File $3 is not readable"
    fi
    methods "$2" "$3" $4;;

  *) die_usage "Invalid command $1";;
esac

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
  usage
  exit 0
fi

