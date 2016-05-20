#!/usr/bin/env python
# -*- coding: utf-8 -*-

import difflib
import json
import os
import re
import subprocess
import unittest

TEST_ROOT = os.path.dirname(__file__)
PROJECT_ROOT = os.path.dirname(TEST_ROOT)
HOEDOWN = [os.path.abspath(os.path.join(PROJECT_ROOT, 'hoedown'))]
TIDY = ['tidy', '--show-body-only', '1', '--show-warnings', '0',
        '--quiet', '1']
CONFIG_PATH = os.path.join(TEST_ROOT, 'config.json')
SLUGIFY_PATTERN = re.compile(r'\W')


def with_metaclass(meta, *bases):
    """Metaclass injection utility from six.

    See: https://pythonhosted.org/six/
    """
    class metaclass(meta):
        def __new__(cls, name, this_bases, d):
            return meta(name, bases, d)
    return type.__new__(metaclass, 'temporary_class', (), {})


class TestFailed(AssertionError):
    def __init__(self, name, expected, got):
        super(TestFailed, self).__init__(self)
        diff = difflib.unified_diff(
            expected.splitlines(), got.splitlines(),
            fromfile='Expected', tofile='Got',
        )
        self.description = '{name}\n{diff}'.format(
            name=name, diff='\n'.join(diff),
        )

    def __str__(self):
        return self.description


def _test_func(test_case):
    flags = test_case.get('flags') or []
    hoedown_proc = subprocess.Popen(
        HOEDOWN + flags + [os.path.join(TEST_ROOT, test_case['input'])],
        stdout=subprocess.PIPE,
    )
    stdoutdata = hoedown_proc.communicate()[0]

    got_tidy_proc = subprocess.Popen(
        TIDY, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
    )
    got = got_tidy_proc.communicate(input=stdoutdata)[0].strip()

    expected_tidy_proc = subprocess.Popen(
        TIDY + [os.path.join(TEST_ROOT, test_case['output'])],
        stdout=subprocess.PIPE,
    )
    expected = expected_tidy_proc.communicate()[0].strip()

    # Cleanup.
    hoedown_proc.stdout.close()
    got_tidy_proc.stdout.close()
    expected_tidy_proc.stdout.close()

    try:
        assert expected == got
    except AssertionError:
        raise TestFailed(test_case['input'], expected, got)


def _make_test(test_case):
    return lambda self: _test_func(test_case)


class MarkdownTestsMeta(type):
    """Meta class for ``MarkdownTestCase`` to inject test cases on the fly.
    """
    def __new__(meta, name, bases, attrs):
        with open(CONFIG_PATH) as f:
            config = json.load(f)

        for test in config['tests']:
            input_name = test['input']
            attr_name = 'test_' + SLUGIFY_PATTERN.sub(
                '_', os.path.splitext(input_name)[0].lower(),
            )
            func = _make_test(test)
            func.__doc__ = input_name
            if test.get('skip', False):
                func = unittest.skip(input_name)(func)
            if test.get('fail', False):
                func = unittest.expectsFailure(func)
            attrs[attr_name] = func
        return type.__new__(meta, name, bases, attrs)


class MarkdownTests(with_metaclass(MarkdownTestsMeta, unittest.TestCase)):
    pass


if __name__ == '__main__':
    unittest.main()
