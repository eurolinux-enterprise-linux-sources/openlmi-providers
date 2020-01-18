from setuptools import setup
from lmi.pcp import __version__

setup(
    name='openlmi-pcp',
    description='PCP metric providers',
    author='Frank Ch. Eigler',
    author_email='fche@redhat.com',
    url='https://fedorahosted.org/openlmi/',
    version=__version__,
    namespace_packages=['lmi'],
    packages=[
        'lmi',
        'lmi.pcp'],
    install_requires=['openlmi', 'pcp'],
    license="LGPLv2+",
    classifiers=[
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',
        'Operating System :: POSIX :: Linux',
        'Topic :: System :: Systems Administration',
        ]
    )
