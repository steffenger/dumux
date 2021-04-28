#!/usr/bin/env python3

"""
Build and/or run (using `dune-ctest`) a selection of tests.
Run this in the top level of the build tree.
"""

import sys
import json
import subprocess
from argparse import ArgumentParser

# require Python 3
if sys.version_info.major < 3:
    sys.exit('Python 3 required')


def buildTests(config, flags=['j4']):

    if not config:
        print('No tests to be built')
        return

    # The MakeFile generated by cmake contains the .NOTPARALLEL statement, as
    # it only allows one call to `CMakeFiles/Makefile2` at a time. Parallelism
    # is taken care of within that latter Makefile. Therefore, we create a
    # small custom Makefile here on top of `Makefile2`, where we define a new
    # target, composed of affected tests, that can be built in parallel
    with open('TestMakeFile', 'w') as makeFile:
        # include make file generated by cmake
        makeFile.write('include CMakeFiles/Makefile2\n')

        # define a new target composed of the test targets
        makeFile.write('testselection: ')
        makeFile.write(' '.join([tc['target'] for tc in config.values()]))

    subprocess.run(['make', '-f', 'TestMakeFile'] + flags + ['testselection'],
                   check=True)


def runTests(config, script='', flags=['-j4', '--output-on-failure']):

    tests = list(config.keys())
    if not tests:
        print('No tests to be run. Letting dune-ctest produce empty report.')
        tests = ['NOOP']

    # if not given, try system-wide call to dune-ctest
    call = ['dune-ctest'] if not script else ['./' + script.lstrip('./')]
    call.extend(flags)
    call.extend(['-R'] + tests)
    subprocess.run(call, check=True)


if __name__ == '__main__':
    parser = ArgumentParser(description='Build or run a selection of tests')
    parser.add_argument('-a', '--all',
                        required=False,
                        action='store_true',
                        help='use this flag to build/run all tests')
    parser.add_argument('-c', '--config',
                        required=False,
                        help='json file with configuration of tests to be run')
    parser.add_argument('-s', '--script',
                        required=False,
                        default='',
                        help='provide the path to the dune-ctest script')
    parser.add_argument('-b', '--build',
                        required=False,
                        action='store_true',
                        help='use this flag to build the tests')
    parser.add_argument('-t', '--test',
                        required=False,
                        action='store_true',
                        help='use this flag to run the tests')
    parser.add_argument('-bf', '--buildflags',
                        required=False,
                        default='-j4',
                        help='set the flags passed to make')
    parser.add_argument('-tf', '--testflags',
                        required=False,
                        default='-j4 --output-on-failure',
                        help='set the flags passed to ctest')
    args = vars(parser.parse_args())

    if not args['build'] and not args['test']:
        sys.exit('Neither `build` not `test` flag was set. Exiting.')

    if args['config'] and args['all']:
        sys.exit('Error: both `config` and `all` specified. '
                 'Please set only one of these arguments.')

    # prepare build and test flags
    buildFlags = args['buildflags'].split(' ')
    testFlags = args['testflags'].split(' ')

    # use target `all`
    if args['all']:
        if args['build']:
            print('Building all tests')
            subprocess.run(['make'] + buildFlags + ['build_tests'], check=True)
        if args['test']:
            print('Running all tests')
            subprocess.run(['ctest'] + testFlags, check=True)

    # use target selection
    else:
        with open(args['config']) as configFile:
            config = json.load(configFile)
            numTests = len(config)
            print('{} tests found in the configuration file'.format(numTests))

            if args['build']:
                buildTests(config, buildFlags)
            if args['test']:
                runTests(config, args['script'], testFlags)
