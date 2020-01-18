from setuptools import setup
from lmi.base import __version__

setup(
    name='openlmi',
    description='OpenLMI python providers',
    author='Michal Minar',
    author_email='miminar@redhat.com',
    url='https://fedorahosted.org/openlmi/',
    version=__version__,
    license='LGPLv2+',
    namespace_packages = ['lmi'],
    packages = ['lmi', 'lmi.base', 'lmi.providers'],
    install_requires=['pywbem'],
    classifiers=[
        'License :: OSI Approved :: GNU Lesser General Public License'
            ' v2 or later (LGPLv2+)',
        'Operating System :: POSIX :: Linux',
        'Topic :: System :: Systems Administration',
        ]
    )
