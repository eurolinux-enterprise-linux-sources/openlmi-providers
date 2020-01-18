from setuptools import setup
from lmi.software import __version__

setup(
    name='openlmi-software',
    description='Software Management providers',
    author='Michal Minar',
    author_email='miminar@redhat.com',
    url='https://fedorahosted.org/openlmi/',
    version=__version__,
    namespace_packages=['lmi'],
    packages=[
        'lmi',
        'lmi.software',
        'lmi.software.core',
        'lmi.software.util',
        'lmi.software.yumdb'],
    data_files=[('/etc/openlmi/software',
            ['config/software.conf', 'config/yum_worker_logging.conf'])],
    install_requires=['openlmi'],
    license="LGPLv2+",
    classifiers=[
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',
        'Operating System :: POSIX :: Linux',
        'Topic :: System :: Systems Administration',
        ]
    )
