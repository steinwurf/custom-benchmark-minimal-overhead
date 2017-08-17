#! /usr/bin/env python
# encoding: utf-8

APPNAME = 'cbmo'
VERSION = '0.0.0'

def build(bld):

    bld.env.append_unique(
        'DEFINES_STEINWURF_VERSION',
        'STEINWURF_CBMO_VERSION="{}"'.format(
            VERSION))

    bld.program(
        features='cxx benchmark',
        source=bld.path.ant_glob('src/cbmo/cbmo.cpp'),
        target='cbmo',
        use=['gauge', 'kodo_rlnc_includes'])
