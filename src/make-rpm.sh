#!/bin/sh

PKG="openlmi-providers"

dir="$(dirname $(realpath $0))"

"$dir/make-release.sh" $1 || exit 1

if [ $# -lt 1 ];
then
    # We want current dirty copy
    tag="--dirty"
else
    tag="$1"
fi

VERSION="$(git describe $tag | sed 's/-/_/g')"

tempdir="$(mktemp -d)"
mkdir -p "$tempdir"
trap "rm -rf $tempdir" EXIT

sed "s/^Version:.*$/Version: $VERSION/g" "$dir/openlmi-providers.spec" > "$tempdir/openlmi-providers.spec"
rpmdev-bumpspec -c "Version $VERSION" "$tempdir/openlmi-providers.spec"

rpmbuild --define "_sourcedir $PWD" -ba "$tempdir/openlmi-providers.spec"
