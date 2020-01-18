#!/usr/bin/env python
#
# Copyright (C) 2012-2013 Red Hat, Inc.  All rights reserved.
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
# Authors: Michal Minar <miminar@redhat.com>
#
"""
Module defining and creating testing packages and repositories for
OpenLMI Software provider.
"""

import collections
import datetime
import functools
import json
import os
import shutil
import stat
import subprocess
import tempfile
import time
import yum

import package
import repository
import util

REGULAR, DOC, CONFIG, LINK, CHARDEV, BLOCKDEV, FIFO = [
        2**i for i in range(7) ]

#: Holds variables for yum config.
#: * ``releasever`` - version of distribution ('18' for *Fedora 18*).
#: * ``disttag`` - dist tag of distribution. This is a part of release string
#:                 of packages build for particular distribution
#:                 ('.fc18' for *Fedora 18*).
#: * ``basearch`` - target architecture ('i686', 'x86_64', etc.).
#: * ``arch`` - system architecture ('ia32e')
YumVars = collections.namedtuple('YumVars',
        ('releasever', 'disttag', 'basearch', 'arch'))

#: Dictionary with shortned package names as keys with assigned contents.
#: Content represented with recursive dictionaries where dictionaries
#: represent folders and tuples files, pipes, symlinks, etc.
#: Directories whose names end with '/' will be owned.
PKG_FILE_ENTRIES = {
    'pkg1' : {
        'usr/share/openlmi-sw-test-pkg1/' : {
            'README'    : (REGULAR | DOC, 0644,  'This is a test pkg 1\n'),
            'Changelog' : (REGULAR | DOC, 0644, 'Nothing new\n'),
            'data/' : { 'file.txt' : (REGULAR, 0644, 'content\n') }
        },
        'etc/openlmi/software/test/' : {
            'dummy_config.cfg' :
                (REGULAR | CONFIG, 0644, '# This is a dummy cfg file\n'),
        }
    },

    'pkg2' : {
        'usr/share/openlmi-sw-test-pkg2/' : {
            'README'    : (REGULAR | DOC, 0644, 'This is a test pkg 2\n'),
            'Changelog' : (REGULAR | DOC, 0644, 'Nothing new\n'),
            'data/' : {
                'target.txt' : (REGULAR, 0644, 'Target of symlinks\n')
            },
            'symlinks/' : {
                'relative' : (LINK, 0644, '../data/target.txt'),
                'absolute' : (LINK, 0644,
                    'usr/share/openlmi-sw-text-pkg1/data/target.txt')
            }
        },
    },

    'pkg3' : {
        'usr/share/openlmi-sw-test-pkg3/' : {
            'README'    : (REGULAR | DOC, 0644, 'This is a test pkg 3\n'),
            'Changelog' : (REGULAR | DOC, 0644, 'Nothing new\n'),
            'fifos/'    : {
                'pipe' : (FIFO, 0644, None)
            },
            'devs/'     : {
                'char12'  : (CHARDEV, 0644, (1, 2)),
                'char34'  : (CHARDEV, 0644, (3, 4)),
                'block56' : (BLOCKDEV, 0644, (5, 6)),
                'block78' : (BLOCKDEV, 0644, (7, 8))
            }
        }
    },

    'pkg4' : {
        'usr/bin' : {
            'openlmi-sw-test-script' :
                (REGULAR, 0755, '#!/bin/sh\necho "OpenLMI rules!"')
        },
        'usr/share/openlmi-sw-test-pkg4/' : {
            'README'    : (REGULAR | DOC, 0644, 'This is a test pkg 4\n'),
            'Changelog' : (REGULAR | DOC, 0644, 'Nothing new\n'),
            'perms/' : {    # this will be given 777 perms with the sticky bit
                'rwxrwxrwx' : (REGULAR, 0777, ''),
                'rw-rw-rw-' : (REGULAR, 0666, ''),
                'r--r--r--' : (REGULAR, 0444, ''),
                'r-xr-xr-x' : (REGULAR, 0555, ''),
                'r---w---x' : (REGULAR, 0421, ''),
                'rwSr--r--' : (REGULAR, 04644, ''),
                'rw-r-Sr--' : (REGULAR, 02644, ''),
                'rw-r--r-T' : (REGULAR, 01644, ''),
                'rwsr-sr-t' : (REGULAR, 07755, ''),
            }
        },
    }
}

#: Repositories with assigned packages.
REPOS = {
    'openlmi-sw-test-repo-stable' : {
        # grows by revision
        'openlmi-sw-test-pkg1-0:1.2.3-1%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg1']
        },
        # grows by epoch
        'openlmi-sw-test-pkg2-1:1.2.3-1%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg2']
        },
        # grows by version
        'openlmi-sw-test-pkg3-0:1.2.1-3%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg3']
        },
        # no update available
        'openlmi-sw-test-pkg4-1:1.2.3-1%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg4']
        }
    },

    'openlmi-sw-test-repo-updates' : {
        'openlmi-sw-test-pkg1-0:1.2.3-2%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg1']
        },
        'openlmi-sw-test-pkg2-2:1.2.2-1%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg2']
        },
        'openlmi-sw-test-pkg3-0:1.2.2-2%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg3']
        },
        # exactly the same version as in repo stable
        'openlmi-sw-test-pkg4-1:1.2.3-1%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg4']
        }
    },

    'openlmi-sw-test-repo-updates-testing' : {
        'openlmi-sw-test-pkg1-0:1.2.3-3%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg1']
        },
        'openlmi-sw-test-pkg2-3:1.2.0-1%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg2']
        },
    },

    'openlmi-sw-test-repo-misc' : {
        # this package conflicts with pkg1 (explicitely)
        'openlmi-sw-test-conflict1-0:1.2.3-1%(disttag)s.noarch' : {
            'conflicts' : ['openlmi-sw-test-pkg1'],
            'files' : {
                'usr/share/openlmi-sw-test-conflict1/' : {
                    'README'    : (REGULAR | DOC, 0644,  ''),
                }
            }
        },
        # this package conflicts with pkg2 (by files)
        'openlmi-sw-test-conflict2-0:1.2.3-1%(disttag)s.noarch' : {
            'files' : PKG_FILE_ENTRIES['pkg1']
        },
        # this package depends on few packages above
        'openlmi-sw-test-depend1-0:1.2.3-1%(disttag)s.noarch' : {
            'requires' : ['openlmi-sw-test-pkg1', 'openlmi-sw-test-pkg2'],
            'files' : {
                'usr/share/openlmi-sw-test-depend1/' : {
                    'README'    : (REGULAR | DOC, 0644,  ''),
                }
            }
        },
        # same as previous one, with version requirements
        'openlmi-sw-test-depend2-0:1.2.3-1%(disttag)s.noarch' : {
            'requires' : [
                'openlmi-sw-test-pkg1 = 1.2.3-1%(disttag)s',
                'openlmi-sw-test-pkg2 = 2:1.2.2-1%(disttag)s'],
            'files' : {
                'usr/share/openlmi-sw-test-depend2/' : {
                    'README'    : (REGULAR | DOC, 0644,  ''),
                }
            }
        },
        # same package as stable#pkg2 except for arch and shared
        # directory
        'openlmi-sw-test-pkg2-1:1.2.3-1%(disttag)s.%(basearch)s' : {
            'files' : {
                'usr/share/openlmi-sw-test-pkg2-arch/' :
                    PKG_FILE_ENTRIES['pkg2'].values()[0]
            }
        },
        # funny (but valid) version string
        'openlmi-sw-test-funny-version-0:.1bA.+~-1%(disttag)s.noarch' : {
            'files' : {
                'usr/share/openlmi-sw-test-funny-version/' : {
                    'README'    : (REGULAR | DOC, 0644,  ''),
                }
            }
        },
        # funny (but valid) release string
        'openlmi-sw-test-funny-release-0:1.2.3-+.rel%(disttag)s.noarch' : {
            'files' : {
                'usr/share/openlmi-sw-test-funny-release/' : {
                    'README'    : (REGULAR | DOC, 0644,  ''),
                }
            }
        },
    },
}

SPEC_TEMPLATE = """\
Name:     %(pkg_name)s
Epoch:    %(epoch)s
Version:  %(version)s
Release:  %(release)s
License:  LGPLv2+
Source0:  %(pkg_name)s-%(epoch)s:%(version)s-%(release)s.%(pkg_arch)s.tar.gz
%(arch_string)s
Requires: openlmi-software
%(requires_string)s
Summary: This is a test package %(pkg_name)s

%%description
Test package for OpenLMI Software provider named %(pkg_name)s.

%%prep
%%setup -q

%%build
# no build necessary

%%install
%(install_string)s

%%files
%(files_string)s

%%changelog
* Mon Nov 04 2013 Michal Minar <miminar@redhat.com> 0.4.0-2
- initial commit
"""

REPO_CONFIG_TEMPLATE = """\
[%(repoid)s]
name=%(name)s
baseurl=file://%(repos_dir)s/%(repoid)s
gpgcheck=0
enabled=1
"""

def full_repo_name(repoid):
    """
    Convenient function returning full repository name in case it's in
    shortened form. Full repository name starts with ``openlmi-sw-test-repo-``.

    :param string repoid: Repository id either in shortened or full form.
        Must be present in ``REPOS``.
    :returns: Full repository name.
    :rtype: string
    """
    if not repoid in REPOS:
        if not repoid.startswith('openlmi-sw-test-repo-'):
            repoid = 'openlmi-sw-test-repo-' + repoid
    if not repoid in REPOS:
        raise ValueError('invalid repoid: %s' % repoid)
    return repoid

def full_pkg_name(pkg_name):
    """
    Convenient function returning full package name in case it's in
    shortened form. Full package name starts with ``openlmi-sw-test-``.

    :param string pkg_name: Package name either in shortened or full form.
        Must be present in one of testing repositories.
    :returns: full_pkg_name
    :rtype: string
    """
    if not pkg_name.startswith('openlmi-sw-test-'):
        pkg_name = 'openlmi-sw-test-' + pkg_name
    return pkg_name

def _unwind_files(path, files_dict):
    """
    Turns package files dictionary into a list of installed files.
    Called recursively until all entries from *files_dict* are handled.

    :param string path: Path where files are nested. If called the first time,
        it should be ``'/'``.
    :param dictionary files_dict: One of files dictionary from
        ``PKG_FILE_ENTRIES`` or one with the same format.
    :returns: List of absolute file paths of all package entries.
    :rtype: list
    """
    result = []
    for name, entry in files_dict.items():
        full_path = os.path.join(path, name)
        if isinstance(entry, dict):
            if name.endswith('/'):
                result.append(full_path[:-1])
            result.extend(_unwind_files(full_path, entry))
        else:
            result.append(full_path)
    return result

def _make_package_object(repo, pkg_nevra, rpm_path, pkg_dict):
    """
    Create package object with supplied informations. RPM package needs
    to be created first.

    :param repo: Instance of repository.
    :param string pkg_nevra: NEVRA of package to create.
    :param string rpm_path: Absolute path to rpm file.
    :param dictionary pkg_dict: One of ``REPOS`` values.
    :returns: Package object.
    :rtype: :py:class:`package.Package`
    """
    match = util.RE_NEVRA.match(pkg_nevra)
    pkg = package.Package(match.group('name'),
            epoch=match.group('epoch'),
            ver=match.group('ver'),
            rel=match.group('rel'),
            arch=match.group('arch'),
            repoid=repo.repoid,
            rpm_path=rpm_path,
            files=_unwind_files('/', pkg_dict.get('files', {})))
    return pkg

def _write_package_directory(path, entries=None):
    """
    Write contents of package to some directory. This content may then be
    archived into a source tarball.

    :param string path: Path to directory where the package contents shall
        be written.
    :param dictionary entries: File dictionary.
    """
    if entries is None:
        entries = {}
    os.makedirs(path)
    if path.endswith('perms/'):
        os.chmod(path,  0777 | stat.S_ISVTX)
    for name, entry in entries.items():
        full_path = os.path.join(path, name)
        if isinstance(entry, dict):
            _write_package_directory(full_path, entry)
        elif isinstance(entry, tuple):
            (ftype, perms, content) = entry
            if ftype & REGULAR:
                with open(full_path, 'w') as outf:
                    outf.write(content)
            elif ftype & LINK:
                os.symlink(content, full_path)
            elif ftype & (CHARDEV | BLOCKDEV):
                mode = ( (stat.S_IFCHR if ftype & CHARDEV else stat.S_IFBLK)
                       | perms)
                os.mknod(full_path, mode, os.makedev(content[0], content[1]))
            elif ftype & FIFO:
                os.mkfifo(full_path, perms)
            else:
                raise ValueError('file type 0%o unknown for file "%s"'
                        % (ftype, full_path))
            if not ftype & (CHARDEV | BLOCKDEV | FIFO | LINK):
                os.chmod(full_path, perms)
        else:
            raise TypeError('enexpected entry in package dictionary: "%s"'
                    % repr(entry))

def _make_source_tarball(rpmbuild_dir, pkg_nevra, pkg_dict):
    """
    Create a source tarball for rpm.

    :param string rpmbuild_dir: Path to directory where ``rpmbuild`` shall
        operate.
    :param string pkg_nevra: NEVRA string of package to build.
    :param dictionary pkg_dict: One of values of ``REPOS`` dictionary.
    :returns: Absolute file path to created tarball.
    :rtype: string
    """
    sources_dir = os.path.join(rpmbuild_dir, 'SOURCES')
    if not os.path.exists(sources_dir):
        os.mkdir(sources_dir)
    match = util.RE_NEVRA.match(pkg_nevra)
    tarball_path = os.path.join(sources_dir, pkg_nevra + '.tar.gz')
    archive_dir = tempfile.mkdtemp()
    try:
        src_dir = os.path.join(archive_dir,
                '%s-%s' % (match.group('name'), match.group('ver')))
        _write_package_directory(src_dir, pkg_dict.get('files', {}))
        subprocess.call(['/usr/bin/tar', '-C', archive_dir,
            '-czf', tarball_path, os.path.basename(src_dir)])
        return tarball_path
    finally:
        shutil.rmtree(archive_dir)

def _make_spec_files_string(path, entries):
    """
    Make a string used in spec file template to define a list of files
    belonging to package. Called recursively.

    :param string path: Absolute path to an entry to be added.
    :param dictionary entries: Dictionary representing directory content to
        include in resulting list.
    :returns: List of lines. Each representing one file or directory.
    :rtype: list
    """
    flag_to_prefix = {
            DOC : '%doc ',
            CONFIG : '%config(noreplace) ',
            0 : ''
    }
    res = []
    for name, entry in entries.items():
        full_path = os.path.join(path, name)
        if isinstance(entry, dict):
            if name.endswith('perms/'):
                res.append('%dir %attr(1777, -, -) ' + full_path)
            elif name.endswith('/'):
                res.append('%dir ' + full_path)
            res.extend(_make_spec_files_string(full_path, entry))
        elif isinstance(entry, tuple):
            if REGULAR & entry[0]:
                attrs = '%%attr(%o, -, -) ' % entry[1]
            else:
                attrs = ''
            res.append(flag_to_prefix[entry[0] & (DOC | CONFIG)]
                    + attrs + full_path)
        else:
            raise TypeError('enexpected entry in package dictionary: "%s"'
                    % repr(entry))
    return res

def _get_rpm_name_from_spec(spec_file_path):
    """
    :param string spec_file_path: Absolute file path to spec file.
    :returns: Name of rpm file builded out of particular spec file.
    :rtype: string
    """
    return subprocess.check_output(['/usr/bin/rpm', '-q', '--specfile',
        spec_file_path]).splitlines()[0] + '.rpm'

def _build_pkg(rpmbuild_dir, pkg_nevra, pkg_dict):
    """
    Build RPM package.

    :param string rpmbuild_dir: Absolute path to directory where
        the build takes place.
    :param string pkg_nevra: Nevra string of package to build.
    :param dictionary pkg_dict: One of ``REPOS`` values.
    :returns: Absolute file path to created rpm.
    :rtype: string
    """
    config = get_yum_config()
    specs_dir = os.path.join(rpmbuild_dir, 'SPECS')
    build_dir = os.path.join(rpmbuild_dir, 'BUILD')
    rpm_dir = os.path.join(rpmbuild_dir, 'RPMS')
    if not os.path.exists(specs_dir):
        os.mkdir(specs_dir)
    if not os.path.exists(build_dir):
        os.mkdir(build_dir)
    spec_path = os.path.join(specs_dir, pkg_nevra + '.spec')
    match = util.RE_NEVRA.match(pkg_nevra)
    if not match:
        raise ValueError('pkg_nevra is not a valid nevra: "%s"' % pkg_nevra)
    with open(spec_path, 'w') as spec:
        spec_args = {
            "pkg_name" : match.group('name'),
            "epoch"    : match.group('epoch'),
            "version"  : match.group('ver'),
            "release"  : match.group('rel'),
            "pkg_arch" : match.group('arch')
        }
        if match.group('arch') != get_yum_config().basearch:
            spec_args['arch_string'] = 'BuildArch: %s\n' % match.group('arch')
        else:
            spec_args['arch_string'] = ''
        for deps in ('requires', 'conflicts'):
            if deps in pkg_dict and pkg_dict[deps]:
                spec_args[deps + '_string'] = "\n".join(
                        ('%s: %s' % (deps.capitalize(), r))
                    for r in pkg_dict[deps]) % config._asdict()
            else:
                spec_args[deps + '_string'] = ''
        spec_args['files_string'] = "\n".join(
            _make_spec_files_string('/', pkg_dict.get('files', {})))
        if pkg_dict.get('files', []):
            spec_args['install_string'] = \
                    'mkdir -p ${RPM_BUILD_ROOT}\ncp -a ./* ${RPM_BUILD_ROOT}/'
        else:
            spec_args['install_string'] = ''
        spec.write(SPEC_TEMPLATE % spec_args)

    subprocess.call(['/usr/bin/rpmbuild', '--quiet',
        '-ba',                              # build rpm package
        '-D', '_topdir %s' % rpmbuild_dir,  # build in rpmbuild_dir
        spec_path                           # from this spec
        ])
    return os.path.join(rpm_dir, match.group('arch'),
        _get_rpm_name_from_spec(spec_path))

def _make_pkg(rpmbuild_dir, pkg_nevra, pkg_dict):
    """
    :returns: Absolute path to created rpm package.
    :rtype: string
    """
    _make_source_tarball(rpmbuild_dir, pkg_nevra, pkg_dict)
    return _build_pkg(rpmbuild_dir, pkg_nevra, pkg_dict)

def _create_repo(repos_dir, repoid, packages):
    """
    :returns: Absolute path to repository configuration file.
    :rtype: string
    """
    config_path = os.path.join('/etc/yum.repos.d', repoid + '.repo')
    repo_dir = os.path.join(repos_dir, repoid)
    with open(config_path, 'w') as config_file:
        config_file.write(REPO_CONFIG_TEMPLATE % {
            'repoid'    : repoid,
            'repos_dir' : repos_dir,
            'name'      : 'OpenLMI Software Test Repository - ' + ' '.join(
                        r.capitalize()
                    for r in repoid[len('openlmi-sw-test-repo-'):].split('-'))
        })
    os.makedirs(repo_dir)
    for rpm_path in packages:
        shutil.move(rpm_path, repo_dir)
    subprocess.call(['/usr/bin/createrepo', '--quiet', repo_dir])
    return config_path

def get_yum_config():
    """
    :returns: Yum configuration variables for this system.
    :rtype: :py:class:`YumVars`
    """
    if not hasattr(get_yum_config, '_yum_vars'):
        yumvar_dict = yum.YumBase().conf.yumvar
        kwargs = {   f: yumvar_dict[f]
                 for f in YumVars._fields if f in yumvar_dict}
        kwargs['disttag'] = '.' + subprocess.check_output(
                ['/usr/bin/rpm', '-q', '--qf', '%{RELEASE}\n'
                , 'kernel']).splitlines()[0].split('.')[-1]
        get_yum_config._yum_vars = YumVars(**kwargs)
    return get_yum_config._yum_vars

def make_repositories(repos_dir):
    """
    :returns: ``{ repoid : (config_path, rpms), ... }``
        Where rpms is a list of pairs:
            ``[ (pkg_nevra, rpm_path), ... ]``
    :rtype: dictionary
    """
    rpmbuild_dir = tempfile.mkdtemp()
    config = get_yum_config()
    try:
        # (repoid, (config_file_path, rpm_package_list))
        result = {}
        for repoid, packages in REPOS.items():
            rpms = []
            for pkg_tmpl, pkg_dict in packages.items():
                pkg_nevra = pkg_tmpl % config._asdict()
                rpms.append(
                        ( pkg_nevra
                        , _make_pkg(rpmbuild_dir, pkg_nevra, pkg_dict)))
            result[repoid] = (
                    _create_repo(repos_dir, repoid, [r for _, r in rpms]),
                    rpms)
    finally:
        shutil.rmtree(rpmbuild_dir)
    return result

def make_object_database(repos_dir):
    """
    :returns: ``(test_repo_db, other_repo_db)`` where
        both items have the following structure: ``{ repoid : repo, ... }``.
        First item contains repositories that shall be used for testing.
        Others are currently present on system and will be disabled during
        tests.
    :rtype: tuple
    """
    repos = make_repositories(repos_dir)
    db = dict((r.repoid, r) for r in repository.get_repo_database())
    result = {}
    for repoid, (_config_path, rpms) in repos.items():
        repo = db.pop(repoid)
        for pkg_nevra, rpm_path in rpms:
            pkg_name = util.RE_NEVRA.match(pkg_nevra).group('name')
            pkg_dict = [  d for p, d in REPOS[repoid].items()
                       if p.startswith(pkg_name)][0]
            pkg = _make_package_object(repo, pkg_nevra,
                    os.path.join(repo.local_path, os.path.basename(rpm_path)),
                    pkg_dict)
            repo.packages.add(pkg)
            result[repoid] = repo
    return (result, db)

class ObjectEncoder(json.JSONEncoder):
    """
    Takes care of encoding repository and package objects for json to handle
    library to handle.
    """
    def default(self, obj):
        if isinstance(obj, repository.Repository):
            return repository.to_json(self, obj)
        elif isinstance(obj, package.Package):
            return package.to_json(self, obj)
        elif isinstance(obj, datetime.datetime):
            return int(time.mktime(obj.timetuple()))
        elif isinstance(obj, set):
            return list(obj)
        return json.JSONEncoder.default(self, obj)

def save_test_database(repos_dir, test_repo_db, other_repo_db, dest_path=None):
    """
    Writes package database into a file.

    :param string repos_dir: Directory were repositories are stored.
    :param dictionary test_repo_db: Dictionary with testing repositories.
        It's the first item returned from :py:func:`make_object_database`.
    :param dictionary other_repo_db: Dictionary with other repositories.
        It's the second item returned from :py:func:`make_object_database`.
    :param string dest_path: Absolute file path, where the given objects will be
        serialized. If ``None``, some random, temporary file will be created.
    :returns: File path of written data.
    :rtype: string
    """
    if dest_path is None:
        dest_path = tempfile.mkstemp()
        db_file = os.fdopen(dest_path[0], 'w')
        dest_path = dest_path[1]
    else:
        db_file = open(dest_path, 'w')
    try:
        data = (repos_dir, test_repo_db, other_repo_db)
        json.dump(data, db_file, cls=ObjectEncoder,
                sort_keys=True, indent=4, separators=(',', ': '))
    finally:
        db_file.close()
    return dest_path

def load_test_database(src_path):
    """
    This is inverse function to :py:func:`save_pkg_database`.

    :returns: ``(repos_dir, test_repo_db, other_repo_db)`` package lists loaded
        from file.
    :rtype: tuple
    """
    with open(src_path, 'r') as db_file:
        repos_dir, test_db, other_db = json.load(
                db_file,
                object_hook=functools.partial(
                    repository.from_json, package.from_json))
    return repos_dir, test_db, other_db

def main():
    repos_dir = tempfile.mkdtemp()
    print "created repos dir: %s" % repos_dir
    db = make_object_database(repos_dir)
    import pprint
    pprint.pprint(db)

if __name__ == '__main__':
    main()

