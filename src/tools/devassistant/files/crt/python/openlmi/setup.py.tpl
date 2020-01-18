from setuptools import setup

setup(
    name='{{ PROJECT_NAME }}',
    description='{{ PROJECT_NAME }}',
    author='John Doe',
    author_email='john@example.com',
    url='http://example.com/openlmi/',
    version='0.0.1',
    package_dir={'': 'src'},
    namespace_packages = ['lmi'],
    packages=['lmi', 'lmi.{{ PROJECT_NAME }}'],
    classifiers=[
        'License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)',
        'Operating System :: POSIX :: Linux',
        'Topic :: System :: Systems Administration',
    ],
)
