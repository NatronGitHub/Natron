#!/usr/bin/env python3
"""Unit tests of collect-changelog.py: the parser and the inserter.

Run from anywhere:
    python3 -m unittest discover -s tools/utils -p 'test_*.py'
"""

import datetime
import importlib.util
import os
import unittest

_HERE = os.path.dirname(os.path.abspath(__file__))
_DATA = os.path.join(_HERE, 'testdata', 'collect_changelog')


def _load_module():
    # The script's name has a hyphen, so it can't be imported by name.
    spec = importlib.util.spec_from_file_location(
        'collect_changelog', os.path.join(_HERE, 'collect-changelog.py'))
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


cc = _load_module()


def _read(name):
    with open(os.path.join(_DATA, name), encoding='utf-8') as f:
        return f.read()


def _pr(name, number, merged_at, repo=cc.MAIN_REPO):
    return cc.PullRequest(repo, number, _read(name), merged_at)


class ParseTest(unittest.TestCase):

    def test_colon_headings_and_continuation(self):
        section = cc.parse_changelog_section(_read('pr_colon_headings.md'))
        self.assertTrue(section.found)
        self.assertFalse(section.none)
        self.assertEqual(
            [(e.heading, e.text) for e in section.entries],
            [('Changes', '- Node graph: undoing a backdrop resize restores '
              'its exact size.'),
             ('Changes', '- Preferences: an existing change. #1666'),
             ('Known issues',
              '- A third known issue, with its own reference. #1801')])
        self.assertEqual(section.unparsed, [])

    def test_subheadings_code_blocks_and_comments(self):
        section = cc.parse_changelog_section(_read('pr_subheadings.md'))
        self.assertEqual(
            [(e.heading, e.text) for e in section.entries],
            [('New features', '- Python: `Effect.foo()` does bar.'),
             ('Release notes',
              '- A heading that CHANGELOG.md does not have.')])
        self.assertEqual(section.unparsed,
                         ['* a bullet with the wrong marker'])

    def test_none(self):
        section = cc.parse_changelog_section(_read('pr_none.md'))
        self.assertTrue(section.found)
        self.assertTrue(section.none)
        self.assertEqual(section.entries, [])

    def test_missing(self):
        self.assertFalse(
            cc.parse_changelog_section(_read('pr_missing.md')).found)
        self.assertFalse(cc.parse_changelog_section('').found)

    def test_crlf_and_bold_heading(self):
        body = 'x\r\n### CHANGELOG\r\n**Changes:**\r\n- A line. #5\r\n'
        section = cc.parse_changelog_section(body)
        self.assertEqual([(e.heading, e.text) for e in section.entries],
                         [('Changes', '- A line. #5')])

    def test_sub_bullets_colon_subheading_and_rules(self):
        body = ('### CHANGELOG\n#### Changes:\n- Viewer: a.\n  - detail.\n'
                '- Python: b.\n***\n- Not in the section.\n')
        section = cc.parse_changelog_section(body)
        self.assertEqual([(e.heading, e.text) for e in section.entries],
                         [('Changes', '- Viewer: a. - detail.'),
                          ('Changes', '- Python: b.')])
        self.assertEqual(section.unparsed, [])

    def test_several_headings(self):
        body = ('### CHANGELOG\nChanges:\n- Real.\n'
                '<details>\n### CHANGELOG\nnone\n</details>\n')
        prs = [cc.PullRequest(cc.MAIN_REPO, 9, body, '2026-11-24T10:00:00Z')]
        _, problems = cc.collect(prs, _read('CHANGELOG.md'))
        self.assertEqual([(p.pr, p.message) for p in problems],
                         [('#9', '2 "### CHANGELOG" headings; only the last '
                           'one is read')])

    def test_line_before_any_heading(self):
        section = cc.parse_changelog_section('### CHANGELOG\n- Orphan.\n')
        self.assertEqual([(e.heading, e.text) for e in section.entries],
                         [(None, '- Orphan.')])


class ReferenceTest(unittest.TestCase):

    def test_natron(self):
        self.assertEqual(cc.with_reference('- A.', cc.MAIN_REPO, 12),
                         '- A. #12')
        self.assertEqual(cc.with_reference('- A. #3', cc.MAIN_REPO, 12),
                         '- A. #3')
        # Another repository's issue is not a reference to the Natron PR.
        self.assertEqual(
            cc.with_reference('- A (openfx-io#343).', cc.MAIN_REPO, 12),
            '- A (openfx-io#343). #12')

    def test_plugin_repo(self):
        repo = 'NatronGitHub/openfx-io'
        self.assertEqual(cc.with_reference('- A.', repo, 7),
                         '- A. (openfx-io#7)')
        self.assertEqual(cc.with_reference('- A. #3', repo, 7),
                         '- A. openfx-io#3')
        self.assertEqual(cc.with_reference('- A (openfx-io#3).', repo, 7),
                         '- A (openfx-io#3).')
        self.assertEqual(cc.with_reference('- A, Natron#1500.', repo, 7),
                         '- A, Natron#1500. (openfx-io#7)')


class InsertTest(unittest.TestCase):

    def test_collect_fixture(self):
        prs = [
            _pr('pr_colon_headings.md', 1800, '2026-11-24T10:00:00Z'),
            _pr('pr_none.md', 1802, '2026-11-24T11:00:00Z'),
            _pr('pr_missing.md', 1803, '2026-11-24T12:00:00Z'),
            _pr('pr_subheadings.md', 1810, '2026-11-24T13:00:00Z'),
            _pr('pr_plugin.md', 351, '2026-11-24T14:00:00Z',
                repo='NatronGitHub/openfx-io'),
        ]
        new, problems = cc.collect(prs, _read('CHANGELOG.md'))
        self.assertEqual(new, _read('expected_CHANGELOG.md'))
        self.assertEqual(
            [(p.pr, p.message) for p in problems],
            [('#1803', 'no "### CHANGELOG" section'),
             ('#1810', 'line not parsed: * a bullet with the wrong marker'),
             ('#1810', 'heading "Release notes" is not a section of the '
              'topmost version of CHANGELOG.md: - A heading that '
              'CHANGELOG.md does not have. #1810')])

    def test_internal_label_and_none_drop_lines(self):
        body = '### CHANGELOG\nChanges:\n- Preferences: z.\n'
        prs = [cc.PullRequest(cc.MAIN_REPO, 5, body, '2026-11-24T10:00:00Z',
                              ('commercial',)),
               cc.PullRequest(cc.MAIN_REPO, 6, body + 'none\n',
                              '2026-11-24T11:00:00Z')]
        old = _read('CHANGELOG.md')
        new, problems = cc.collect(prs, old, cc.INTERNAL_LABELS)
        self.assertNotIn('Preferences: z.', new)
        self.assertEqual([p.pr for p in problems], ['#5', '#6'])
        new, _ = cc.collect(prs[:1], old)
        self.assertIn('- Preferences: z. #5', new)
        # An internal PR without a section (e.g. Dependabot) is no problem.
        bot = cc.PullRequest(cc.MAIN_REPO, 7, 'Bump x.', '2026-11-24T12:00:00Z',
                             ('dependencies',))
        self.assertEqual(cc.collect([bot], old, cc.INTERNAL_LABELS)[1], [])

    def test_idempotent(self):
        prs = [_pr('pr_colon_headings.md', 1800, '2026-11-24T10:00:00Z')]
        once, _ = cc.collect(prs, _read('CHANGELOG.md'))
        twice, _ = cc.collect(prs, once)
        self.assertEqual(once, twice)

    def test_heading_case_and_empty_section(self):
        text = '# History\n\n## Version 3\n\n### Changes\n\n### Plugins\n'
        new, missing, _ = cc.insert_entries(
            text, [('changes', '- A. #1'), ('PLUGINS', '- B. #2')])
        self.assertEqual(missing, [])
        self.assertEqual(new, '# History\n\n## Version 3\n\n### Changes\n\n'
                         '- A. #1\n\n### Plugins\n\n- B. #2\n')

    def test_only_topmost_version(self):
        text = ('## Version 3\n\n### Changes\n\n- New.\n\n'
                '## Version 2\n\n### Plugins\n\n- Old.\n')
        new, missing, _ = cc.insert_entries(
            text, [('Plugins', '- P.'), ('Changes', '- Old.')])
        self.assertEqual(missing, [('Plugins', '- P.')])
        self.assertEqual(new, '## Version 3\n\n### Changes\n\n- New.\n'
                         '- Old.\n\n## Version 2\n\n### Plugins\n\n- Old.\n')


class ReorganizeTest(unittest.TestCase):

    def test_component_of(self):
        self.assertEqual(cc.component_of('- Python: `a()` does b.'), 'Python')
        self.assertEqual(
            cc.component_of('- Preferences > Plug-ins > Host: the x.'),
            'Preferences')
        self.assertEqual(cc.component_of('- Unix (Linux, macOS): no abort.'),
                         'Unix')
        self.assertEqual(cc.component_of('- RotoPaint, Tracker: labels.'),
                         'RotoPaint')
        # A colon after code, or after a whole sentence, is no component.
        self.assertIsNone(cc.component_of(
            '- Natron no longer crashes after `app.closeProject()`: it was.'))
        self.assertIsNone(cc.component_of(
            '- The bundled Eigen library is updated to 3.4.1, a bug-fix '
            'release: fixes.'))
        self.assertIsNone(cc.component_of('- No component at all. #1'))

    def test_groups_and_merges(self):
        text = '\n'.join([
            '## Version 3', '', '### Changes', '',
            '- Some general change. #1',
            '- Preferences: a. #2',
            '- File dialog: b. #3',
            '- Viewer overlays: c. #4',
            '- Python: d. #5',
            '- Auto-save: e. #6',
            '- Python: d. #7',
            '- Backdrop: f. #8',
            '', '### Plugins', '',
            '- Shadertoy: g (openfx-misc#10).',
            '- WriteFFmpeg: h. (openfx-io#11)',
            '- Some plug-in fix without a bundle. #12',
            '- ReadFFmpeg: i. (openfx-io#13)',
            '- G\'MIC: j (openfx-gmic#14).',
            '', '', '## Version 2', '', '### Changes', '', '- Old.', ''])
        new, missing, _ = cc.insert_entries(
            text, [('Changes', '- Python: k. #9')])
        self.assertEqual(missing, [])
        self.assertEqual(new, '\n'.join([
            '## Version 3', '', '### Changes', '',
            '- Python: d. #5 #7',
            '- Python: k. #9',
            '- Viewer overlays: c. #4',
            '- Backdrop: f. #8',
            '- Preferences: a. #2',
            '- Auto-save: e. #6',
            '- File dialog: b. #3',
            '- Some general change. #1',
            '', '### Plugins', '',
            '- ReadFFmpeg: i. (openfx-io#13)',
            '- WriteFFmpeg: h. (openfx-io#11)',
            '- Shadertoy: g (openfx-misc#10).',
            '- G\'MIC: j (openfx-gmic#14).',
            '- Some plug-in fix without a bundle. #12',
            '', '', '## Version 2', '', '### Changes', '', '- Old.', '']))


class EditorialContentTest(unittest.TestCase):

    def test_section_with_subheadings_is_not_regrouped(self):
        text = '\n'.join([
            '## Version 3', '', '### Changes', '',
            'A preamble.', '', 'Its second paragraph.', '',
            '- Viewer: b. #2', '', '#### Python', '', '- Python: a. #1', '',
            '### Plugins', '', '- WriteFFmpeg: c (openfx-io#3).', ''])
        new, missing, kept = cc.insert_entries(
            text, [('Changes', '- Build: d. #4'),
                   ('Plugins', '- ReadFFmpeg: e (openfx-io#5).')])
        self.assertEqual((missing, kept), ([], ['Changes']))
        self.assertEqual(new, '\n'.join([
            '## Version 3', '', '### Changes', '',
            'A preamble.', '', 'Its second paragraph.', '',
            '- Viewer: b. #2', '', '#### Python', '', '- Python: a. #1', '',
            '- Build: d. #4', '',
            '### Plugins', '', '- ReadFFmpeg: e (openfx-io#5).',
            '- WriteFFmpeg: c (openfx-io#3).', '']))

    def test_subheading_right_after_an_item(self):
        text = '\n'.join([
            '## Version 3', '', '### Plugins', '',
            '- WriteFFmpeg: a. (openfx-io#1)', '#### openfx-misc',
            '- Shadertoy: b (openfx-misc#2).', '- ReadFFmpeg: c (openfx-io#3).',
            '', '### Changes', '', '- Python: a. #1', '', '#### Sub', '',
            '- Viewer: b. #2', ''])
        new, _, kept = cc.insert_entries(text, [])
        self.assertEqual(kept, ['Plugins', 'Changes'])
        self.assertEqual(new, text)


class LastCollectionTest(unittest.TestCase):

    def test_trailers_from_several_commits(self):
        def fake_run(args, **unused_kwargs):
            del args  # Unused.
            return ('\x01aaa\x002026-11-30T10:00:00+00:00\x00'
                    'CHANGELOG: collect #10..#12\x00CHANGELOG: collect '
                    '#10..#12\n\nMerged-until: NatronGitHub/Natron '
                    '2026-11-30T09:00:00Z\n'
                    '\x01bbb\x002026-11-27T10:00:00+00:00\x00'
                    'Fix a typo\x00Fix a typo\n\nCHANGELOG: collect in body\n'
                    '\x01ccc\x002026-11-25T10:00:00+00:00\x00'
                    'CHANGELOG: collect #1..#5\x00CHANGELOG: collect #1..#5'
                    '\n\nMerged-until: NatronGitHub/Natron 2026-11-25T09:00:00Z'
                    '\nMerged-until: NatronGitHub/openfx-io '
                    '2026-11-25T08:00:00Z\n')
        saved = cc._run
        cc._run = fake_run
        try:
            until = cc.last_collection(
                'origin/RB-2.6', 'CHANGELOG.md',
                [cc.MAIN_REPO, 'NatronGitHub/openfx-io',
                 'NatronGitHub/openfx-gmic'])
        finally:
            cc._run = saved
        self.assertEqual(until, {
            cc.MAIN_REPO: cc.to_utc('2026-11-30T09:00:00Z'),
            'NatronGitHub/openfx-io': cc.to_utc('2026-11-25T08:00:00Z'),
            'NatronGitHub/openfx-gmic': cc.to_utc('2026-11-30T10:00:00Z'),
        })

    def test_newest_commit_squashed_is_an_error(self):
        saved = cc._run
        cc._run = lambda args, **kwargs: (
            '\x01bbb\x002026-11-30T10:00:00+00:00\x00CHANGELOG: collect '
            '#3..#9 (#1900)\x00CHANGELOG: collect #3..#9 (#1900)\n'
            '\x01aaa\x002026-11-25T10:00:00+00:00\x00CHANGELOG: collect '
            '#1..#2\x00CHANGELOG: collect #1..#2\n\nMerged-until: '
            'NatronGitHub/Natron 2026-11-25T10:00:00Z\n')
        try:
            with self.assertRaises(SystemExit):
                cc.last_collection('origin/RB-2.6', 'CHANGELOG.md',
                                   [cc.MAIN_REPO])
        finally:
            cc._run = saved

    def test_no_main_trailer_is_an_error(self):
        saved = cc._run
        cc._run = lambda args, **kwargs: (
            '\x01aaa\x002026-11-30T10:00:00+00:00\x00CHANGELOG: collect '
            '(#20)\x00CHANGELOG: collect (#20)\n')
        try:
            with self.assertRaises(SystemExit):
                cc.last_collection('origin/RB-2.6', 'CHANGELOG.md',
                                   [cc.MAIN_REPO])
        finally:
            cc._run = saved


class CommitMessageTest(unittest.TestCase):

    def test_trailers_round_trip(self):
        since = {
            cc.MAIN_REPO: cc.to_utc('2026-11-24'),
            'NatronGitHub/openfx-io': cc.to_utc('2026-11-24'),
            'NatronGitHub/openfx-misc': cc.to_utc('2026-11-24'),
        }
        prs = [
            cc.PullRequest(cc.MAIN_REPO, 1800, '', '2026-11-24T10:00:00Z'),
            cc.PullRequest('NatronGitHub/openfx-io', 351, '',
                           '2026-11-24T11:00:00Z'),
            cc.PullRequest(cc.MAIN_REPO, 1790, '', '2026-11-24T12:00:00Z'),
        ]
        message = cc.commit_message(prs, since)
        self.assertTrue(message.startswith('CHANGELOG: collect #1800..#1790\n'))
        utc = datetime.timezone.utc
        self.assertEqual(cc.parse_merged_until(message), {
            cc.MAIN_REPO: datetime.datetime(2026, 11, 24, 12, tzinfo=utc),
            'NatronGitHub/openfx-io':
                datetime.datetime(2026, 11, 24, 11, tzinfo=utc),
            'NatronGitHub/openfx-misc':
                datetime.datetime(2026, 11, 24, tzinfo=utc),
        })


if __name__ == '__main__':
    unittest.main()
