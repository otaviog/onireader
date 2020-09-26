"""Setup of onireader.
"""

import sys
from setuptools import find_packages

try:
    from skbuild import setup
except ImportError:
    print('scikit-build is required to build from source.', file=sys.stderr)
    print('Please run:', file=sys.stderr)
    print('', file=sys.stderr)
    print('  python -m pip install scikit-build')
    sys.exit(1)



def _forbid_publish():
    argv = sys.argv
    blacklist = ['register', 'upload']

    for command in blacklist:
        if command in argv:
            values = {'command': command}
            print('Command "%(command)s" has been blacklisted, exiting...' %
                  values)
            sys.exit(2)


_forbid_publish()



REQUIREMENTS = [
    'numpy'
]

setup(
    name='onireader',
    version='1.0.1',
    author='Otavio Gomes',
    author_email='otavio.b.gomes@gmail.com',
    zip_safe=False,
    description="OpenNI2's ONI Reader",
    packages=find_packages(),
    install_requires=REQUIREMENTS,
    extras_require={
        'dev': ['Sphinx',
                'sphinx_theme',
                'sphinxcontrib-napoleon',
                'docutils',
                'pylint',
                'autopep8',
                'ipdb',
                'jedi',
                'matplotlib']
        },
    long_description='')
