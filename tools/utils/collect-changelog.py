#!/usr/bin/env python3
"""Collect the CHANGELOG lines of merged pull requests into CHANGELOG.md.

Pull requests don't edit CHANGELOG.md. Each PR description ends with a
section such as:

    ### CHANGELOG

    Changes:
    - Node graph: undoing a backdrop resize restores its exact size.

    #### New features
    - Python: `Effect.foo()` does bar. #1234

or with "none" when the PR needs no entry. The lines are grouped under
headings ("Heading:" lines or "#### Heading" sub-headings) that name the
"### Heading" sections of the topmost version of CHANGELOG.md (for 2.6.0:
New features, Known issues, Changes, Plugins), compared without regard to
case.

The script finds the PRs merged into the base branch since the last
collection, in merge order, with one paginated `gh pr list` query per
repository, parses their CHANGELOG sections and adds the lines to the
matching sections of the topmost version of CHANGELOG.md. A line with no
"#NNN" reference gets " #<PR number>" appended. The lines of a PR whose
section says "none", or that has an internal label (INTERNAL_LABELS), are
dropped. Then, in every section of that version, the lines that differ only
by their trailing references are merged into one, and the lines are grouped
by component (the text before their first ":", see COMPONENTS), or by bundle
and plug-in in the Plugins section; a section with sub-headings or
paragraphs is left as it is, with the new lines at its end. PRs without a
CHANGELOG section, lines under a heading that CHANGELOG.md doesn't have, and
lines that can't be parsed are reported.

Natron's CHANGELOG.md also lists the plug-in repositories' changes: --extra-repo
collects their merged PRs too (base branch master by default). Their lines
go to the Plugins section unless they name another heading; a bare "#NNN"
in them becomes "<repo>#NNN" (e.g. "openfx-io#343"; a Natron reference is
written "Natron#NNN" there), and a line with no reference gets
" (<repo>#<PR number>)" appended.

The last collection is the last commit of the base branch that changed
CHANGELOG.md with a message starting with "CHANGELOG: collect". Its
"Merged-until: <repo> <time>" trailers (written by --write for --commit)
give, for each repository, the merge time of the last PR it collected (the
newest trailer of each repository, over all collection commits). Without a
trailer for Natron (e.g. a squashed collection), the script stops; an
--extra-repo without one starts at the date of the newest collection commit.
--since gives the starting point explicitly, for every repository.

Run it from the top of a Natron checkout, after `git fetch origin`, with
`gh` authenticated:

    tools/utils/collect-changelog.py            # print the diff (dry run)
    tools/utils/collect-changelog.py --write    # edit CHANGELOG.md
    # Editorial pass: rewrite the topmost version for users, by hand.
    tools/utils/collect-changelog.py --commit   # commit CHANGELOG.md as is

--commit uses the message that --write saved in the git directory; it
fetches nothing and pushes nothing. --write refuses to run while
CHANGELOG.md has uncommitted changes (--force overrides). The procedure,
editorial pass included, is in
Documentation/source/maintainers/contributing.rst ("CHANGELOG").
"""

import argparse
import dataclasses
import datetime
import difflib
import json
import os
import re
import subprocess
import sys
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

COLLECT_PREFIX = 'CHANGELOG: collect'
MERGED_UNTIL = 'Merged-until:'
MAIN_REPO = 'NatronGitHub/Natron'
PLUGINS_HEADING = 'Plugins'
# The lines of PRs with these labels are dropped (see --include-internal):
# COMMERCIAL builds are not released, "dependencies" are Dependabot updates.
# type:task and func:doc are not in the list: some of these PRs are
# user-visible (e.g. a new OpenFX host name).
INTERNAL_LABELS = ('commercial', 'dependencies')

_SECTION_RE = re.compile(r'^###\s+CHANGELOG\s*:?\s*$', re.IGNORECASE)
_END_RE = re.compile(r'^(#{1,3}\s|</?details\b|(?:-{3,}|\*{3,}|_{3,})\s*$)',
                     re.IGNORECASE)
_SUBHEADING_RE = re.compile(r'^#{4,6}\s+(?P<name>.+?)\s*#*$')
_COLON_HEADING_RE = re.compile(r'^(?:\*\*)?(?P<name>[^-*\s][^:]*?):(?:\*\*)?$')
_NONE_RE = re.compile(r'^(?:-\s+)?none\.?$', re.IGNORECASE)
_FENCE_RE = re.compile(r'^\s*(```|~~~)')
_COMMENT_RE = re.compile(r'<!--.*?-->', re.DOTALL)
# A bare "#123", not preceded by a repository name as in "openfx-io#343".
_BARE_REF_RE = re.compile(r'(?<![\w/.-])#(\d+)\b')


@dataclasses.dataclass
class Entry:
    """One CHANGELOG line taken from a PR description."""
    heading: Optional[str]  # None if the line came before any heading.
    text: str  # The line, starting with "- ".


@dataclasses.dataclass
class ParsedSection:
    """The CHANGELOG section of one PR description."""
    found: bool = False
    none: bool = False
    entries: List[Entry] = dataclasses.field(default_factory=list)
    unparsed: List[str] = dataclasses.field(default_factory=list)
    headings: int = 0  # Number of "### CHANGELOG" headings.


@dataclasses.dataclass
class PullRequest:
    """A merged PR, as returned by `gh pr list`."""
    repo: str  # "owner/name".
    number: int
    body: str
    merged_at: str  # ISO 8601, UTC.
    labels: Tuple[str, ...] = ()

    @property
    def label(self) -> str:
        """The PR's reference: "#123" for Natron, "openfx-io#123" else."""
        return f'{reference_prefix(self.repo)}#{self.number}'


@dataclasses.dataclass
class Problem:
    """Something the maintainer must look at."""
    pr: str  # PullRequest.label.
    message: str


def reference_prefix(repo: str) -> str:
    """Returns the prefix of issue references to repo in CHANGELOG.md."""
    return '' if repo == MAIN_REPO else repo.split('/')[-1]


def _strip_code_and_comments(body: str) -> List[str]:
    """Returns the lines of body, with HTML comments and fenced code blanked."""
    body = _COMMENT_RE.sub('', body.replace('\r\n', '\n'))
    lines = []
    in_fence = False
    for line in body.split('\n'):
        if _FENCE_RE.match(line):
            in_fence = not in_fence
            lines.append('')
        else:
            lines.append('' if in_fence else line)
    return lines


def parse_changelog_section(body: str) -> ParsedSection:
    """Parses the "### CHANGELOG" section of a PR description.

    The last such heading outside fenced code blocks and HTML comments is
    used. The section ends at the next heading of level 1 to 3, a
    <details> tag, a horizontal rule, or the end of the description.

    Args:
        body: The PR description (Markdown).

    Returns:
        The parsed section; its `found` is False if there is none.
    """
    lines = _strip_code_and_comments(body or '')
    starts = [i for i, line in enumerate(lines)
              if _SECTION_RE.match(line.strip())]
    section = ParsedSection()
    if not starts:
        return section
    section.found = True
    section.headings = len(starts)
    heading = None
    in_entry = False
    for raw in lines[starts[-1] + 1:]:
        line = raw.strip()
        if _END_RE.match(line):
            break
        if not line:
            continue
        if _NONE_RE.match(line):
            section.none = True
            in_entry = False
        elif raw[:1] in (' ', '\t') and in_entry:
            # An indented continuation (or sub-bullet) of the previous line.
            section.entries[-1].text += ' ' + line
        elif line.startswith('- '):
            section.entries.append(Entry(heading, '- ' + line[2:].strip()))
            in_entry = True
        else:
            in_entry = False
            match = (_SUBHEADING_RE.match(line) or
                     _COLON_HEADING_RE.match(line))
            if match:
                heading = match.group('name').strip('* ').rstrip(':').strip()
            else:
                section.unparsed.append(line)
    return section


def with_reference(text: str, repo: str, number: int) -> str:
    """Returns text with a reference to its PR, unless it has one.

    For Natron, a line with a bare "#NNN" keeps it. For another repository
    (e.g. NatronGitHub/openfx-io), a bare "#NNN" is an issue of that
    repository and becomes "openfx-io#NNN"; a line without any
    "openfx-io#NNN" then gets " (openfx-io#<number>)" appended.

    Args:
        text: A CHANGELOG line.
        repo: The repository of the PR ("owner/name").
        number: The PR number.

    Returns:
        The line, with a reference.
    """
    prefix = reference_prefix(repo)
    if not prefix:
        return text if _BARE_REF_RE.search(text) else f'{text} #{number}'
    text = _BARE_REF_RE.sub(lambda m: f'{prefix}#{m.group(1)}', text)
    if re.search(rf'(?<![\w.-]){re.escape(prefix)}#\d+\b', text):
        return text
    return f'{text} ({prefix}#{number})'


def _normalize(text: str) -> str:
    return ' '.join(text.split())


# Natron's components, in the order of their groups in each section; a line's
# component is the text before its first ":" (see component_of()). The
# aliases are other prefixes that belong to the component.
COMPONENTS = (
    ('Python', ('pyplug', 'pyplugs', 'expressions')),
    ('Viewer', ()),
    ('Node graph', ('backdrop', 'group', 'groups')),
    ('Roto/RotoPaint', ('roto', 'rotopaint')),
    ('Tracker', ()),
    ('OpenFX host', ('openfx', 'ofx')),
    ('Read/Write', ('read', 'write', 'readers', 'writers')),
    ('Preferences', ('settings',)),
    ('Build/packaging', ('build', 'cmake', 'packaging', 'installer')),
)
# The plug-in bundles, in the order of their groups in the Plugins section.
BUNDLES = ('openfx-io', 'openfx-misc', 'openfx-arena', 'openfx-gmic')

_PREFIX_RE = re.compile(r'^- (?P<prefix>[^:]{1,60}?):(?:\s|$)')
_PAREN_RE = re.compile(r'\s*\([^)]*\)')
_REF_TOKEN = r'\(?(?:[\w.-]+/)?[\w.-]*#\d+\)?'
_TRAILING_REFS_RE = re.compile(rf'(?:\s+{_REF_TOKEN})+\s*$')
_BUNDLE_RE = re.compile(r'\b(openfx-[\w-]+)#\d+')


def component_of(text: str) -> Optional[str]:
    """Returns the component of a CHANGELOG line, or None if it has none.

    The component is the text before the first ":", without parentheses,
    up to its first ">" or ",", if it has no code and at most six words; e.g.
    "Preferences > Plug-ins: ..." gives "Preferences" and "Unix (Linux,
    macOS): ..." gives "Unix".
    """
    match = _PREFIX_RE.match(text)
    if not match:
        return None
    prefix = _PAREN_RE.sub('', match.group('prefix'))
    component = re.split(r'[>,]', prefix)[0].strip()
    if '`' in component or not 0 < len(component.split()) <= 6:
        return None
    return component


def _component_rank(text: str) -> Tuple[int, str]:
    """Returns the sort key of a line outside the Plugins section."""
    component = component_of(text)
    if component is None:
        return (len(COMPONENTS) + 1, '')
    key = component.casefold()
    for rank, (name, aliases) in enumerate(COMPONENTS):
        names = (name.casefold(),) + aliases
        if any(key == n or key.startswith(n + ' ') for n in names):
            return (rank, '')
    return (len(COMPONENTS), key)


def _plugin_rank(text: str) -> Tuple[int, str, str]:
    """Returns the sort key of a line of the Plugins section."""
    match = _BUNDLE_RE.search(text)
    bundle = match.group(1) if match else ''
    rank = BUNDLES.index(bundle) if bundle in BUNDLES else len(BUNDLES)
    plugin = component_of(text)
    return (rank, bundle, '~' if plugin is None else plugin.casefold())


@dataclasses.dataclass
class _Item:
    """A "- " line of CHANGELOG.md, with its continuation lines."""
    lines: List[str]

    def key(self) -> str:
        """Returns the text without its trailing references, normalized."""
        text = _normalize(' '.join(self.lines))
        return _TRAILING_REFS_RE.sub('', text)

    def merge(self, other: '_Item') -> None:
        """Adds the trailing references of other that this item lacks."""
        mine = _TRAILING_REFS_RE.search(_normalize(' '.join(self.lines)))
        theirs = _TRAILING_REFS_RE.search(_normalize(' '.join(other.lines)))
        have = re.findall(_REF_TOKEN, mine.group(0)) if mine else []
        for token in re.findall(_REF_TOKEN, theirs.group(0)) if theirs else []:
            if token not in have:
                have.append(token)
                self.lines[-1] = self.lines[-1].rstrip() + ' ' + token


@dataclasses.dataclass
class _Section:
    """A "### " section of the topmost version of CHANGELOG.md."""
    name: str
    preamble: List[str]  # Lines before the first item.
    items: List[_Item]
    spaced: bool  # Items are separated by blank lines.
    trailing_blanks: int  # Blank lines before the next heading.
    list_only: bool  # Only items after the preamble: it can be regrouped.
    raw: List[str]  # The lines after the heading, as they were.
    parsed_items: int  # len(items) before new lines were added.


def _parse_section(name: str, body: Sequence[str]) -> _Section:
    """Splits the lines of a section (after its heading) into items.

    An item is a "- " line with the lines that follow it without a blank
    line, or indented. Any other line after the first item (a "#" heading,
    or a paragraph after a blank line) makes the section not list_only.
    """
    preamble = []
    items = []
    gaps = []  # For each item after the first: is a blank line before it?
    list_only = True
    blank = False
    for line in body:
        if line.startswith('- '):
            if items:
                gaps.append(blank)
            items.append(_Item([line]))
        elif not items:
            preamble.append(line)
        elif line.strip():
            # A "#" heading, or a paragraph after a blank line.
            if line.startswith('#') or (blank and line[:1] not in (' ', '\t')):
                list_only = False
            items[-1].lines.extend(['', line] if blank else [line])
        blank = not line.strip()
    while preamble and not preamble[0].strip():
        preamble.pop(0)
    trailing = len(body) - len(_rstrip_blank(body))
    return _Section(name, _rstrip_blank(preamble), items,
                    bool(gaps) and all(gaps), trailing, list_only,
                    list(body), len(items))


def _rstrip_blank(lines: Sequence[str]) -> List[str]:
    lines = list(lines)
    while lines and not lines[-1].strip():
        lines.pop()
    return lines


def _render_section(section: _Section) -> List[str]:
    """Returns the lines of a section, its heading included.

    A section that is not list_only keeps its lines as they were, followed
    by the new items.
    """
    if not section.list_only:
        out = ['### ' + section.name] + _rstrip_blank(section.raw)
        for item in section.items[section.parsed_items:]:
            if section.spaced or len(out) == 1:
                out.append('')
            out += item.lines
        return out + [''] * max(1, section.trailing_blanks)
    out = ['### ' + section.name, '']
    if section.preamble:
        out += section.preamble + ['']
    for i, item in enumerate(section.items):
        if i and section.spaced:
            out.append('')
        out += item.lines
    if not section.items and not section.preamble:
        out.pop()
    return out + [''] * max(1, section.trailing_blanks)


def reorganize(section: _Section) -> None:
    """Merges the items that differ only by references, and groups them.

    Items are grouped by component (see COMPONENTS), or by bundle and
    plug-in in the Plugins section. The sort is stable: the items of a group
    keep their order.
    """
    merged: Dict[str, _Item] = {}
    items = []
    for item in section.items:
        if item.key() in merged:
            merged[item.key()].merge(item)
        else:
            merged[item.key()] = item
            items.append(item)
    is_plugins = section.name.casefold() == PLUGINS_HEADING.casefold()
    rank = _plugin_rank if is_plugins else _component_rank
    section.items = sorted(items, key=lambda i: rank(i.lines[0]))


def insert_entries(
        changelog: str, entries: Sequence[Tuple[str, str]]
) -> Tuple[str, List[Tuple[str, str]], List[str]]:
    """Inserts CHANGELOG lines into the topmost version and reorganizes it.

    Each line is added to the "### " section of the topmost "## " version
    whose heading matches its own, without regard to case. Then, in every
    section of that version, lines that differ only by their trailing
    references (e.g. "#1743" and "#1767") are merged into one line with all
    the references, and the lines are grouped by component (see
    COMPONENTS), or by bundle and plug-in in the Plugins section. Items keep
    their blank-line separation (as in "Known issues"). A section with
    other content than a list (e.g. "#### " sub-headings or paragraphs
    written in the editorial pass) is not regrouped: the new lines are added
    at its end. The rest of the file is unchanged.

    Args:
        changelog: The text of CHANGELOG.md.
        entries: (heading, line) pairs, in merge order.

    Returns:
        The new text, the (heading, line) pairs whose heading is not a
        section of the topmost version, and the names of the sections that
        were not regrouped.
    """
    lines = changelog.split('\n')
    top = next((i for i, line in enumerate(lines) if line.startswith('## ')),
               None)
    if top is None:
        return changelog, list(entries), []
    end = next((i for i in range(top + 1, len(lines))
                if lines[i].startswith(('## ', '# '))), len(lines))
    starts = [i for i in range(top + 1, end) if lines[i].startswith('### ')]
    if not starts:
        return changelog, list(entries), []
    sections = [_parse_section(lines[s][4:].strip(), lines[s + 1:e])
                for s, e in zip(starts, starts[1:] + [end])]
    by_name = {s.name.casefold(): s for s in sections}
    missing = []
    for heading, text in entries:
        section = by_name.get(heading.casefold())
        if section is None:
            missing.append((heading, text))
        else:
            section.items.append(_Item([text]))
    kept = []
    for section in sections:
        if section.list_only:
            reorganize(section)
        else:
            kept.append(section.name)
    body = [line for s in sections for line in _render_section(s)]
    return '\n'.join(lines[:starts[0]] + body + lines[end:]), missing, kept


def collect(prs: Sequence[PullRequest], changelog: str,
            internal_labels: Iterable[str] = ()
            ) -> Tuple[str, List[Problem]]:
    """Collects the CHANGELOG lines of PRs into a CHANGELOG.

    The lines of a PR whose section says "none", or that has one of
    internal_labels, are dropped (and reported, if it has lines).

    Args:
        prs: The merged PRs, in merge order.
        changelog: The text of CHANGELOG.md.
        internal_labels: Labels of the PRs whose lines are dropped.

    Returns:
        The new text, and the problems to report.
    """
    problems = []
    entries = []  # (heading, line, PullRequest)
    for pr in prs:
        section = parse_changelog_section(pr.body)
        internal = sorted(set(pr.labels) & set(internal_labels))
        if internal and not section.entries:
            continue  # E.g. a Dependabot PR: nothing to report.
        if not section.found:
            problems.append(Problem(pr.label, 'no "### CHANGELOG" section'))
            continue
        if section.headings > 1:
            problems.append(Problem(
                pr.label, f'{section.headings} "### CHANGELOG" headings; '
                'only the last one is read'))
        if not section.entries and not section.none:
            problems.append(Problem(
                pr.label, 'empty "### CHANGELOG" section (no line, no "none")'))
        problems.extend(Problem(pr.label, f'line not parsed: {line}')
                        for line in section.unparsed)
        if section.entries and (section.none or internal):
            why = (f'label {", ".join(internal)}, see --include-internal'
                   if internal else 'its section also says "none"')
            problems.append(Problem(
                pr.label, f'{len(section.entries)} line(s) dropped ({why})'))
            continue
        for entry in section.entries:
            text = with_reference(entry.text, pr.repo, pr.number)
            heading = entry.heading
            if heading is None and pr.repo != MAIN_REPO:
                heading = PLUGINS_HEADING
            if heading is None:
                problems.append(Problem(pr.label,
                                        f'line without a heading: {text}'))
            else:
                entries.append((heading, text, pr))
    new_text, missing, kept = insert_entries(
        changelog, [(h, t) for h, t, _ in entries])
    problems.extend(Problem('CHANGELOG.md', f'section "{name}" not regrouped '
                            '(it has sub-headings or paragraphs); new lines '
                            'added at its end') for name in kept)
    # A line is missing when its heading is: report each PR of such a line,
    # also when two PRs have the same line.
    missing_headings = {heading.casefold() for heading, _ in missing}
    for heading, text, pr in entries:
        if heading.casefold() not in missing_headings:
            continue
        problems.append(Problem(
            pr.label, f'heading "{heading}" is not a section of the topmost '
            f'version of CHANGELOG.md: {text}'))
    return new_text, problems


def _run(args: Sequence[str], **kwargs) -> str:
    return subprocess.run(args, check=True, capture_output=True, text=True,
                          **kwargs).stdout


def _git_ref(base: str) -> str:
    """Returns origin/<base> if it exists locally, else base."""
    for ref in (f'origin/{base}', base):
        if subprocess.run(['git', 'rev-parse', '--verify', '-q', ref],
                          capture_output=True, check=False).returncode == 0:
            return ref
    raise SystemExit(f'error: no branch {base} or origin/{base}; run '
                     '"git fetch origin" first')


def to_utc(timestamp: str) -> datetime.datetime:
    """Parses an ISO 8601 timestamp or a YYYY-MM-DD date (UTC by default)."""
    value = datetime.datetime.fromisoformat(timestamp.replace('Z', '+00:00'))
    if value.tzinfo is None:
        value = value.replace(tzinfo=datetime.timezone.utc)
    return value.astimezone(datetime.timezone.utc)


def parse_merged_until(message: str) -> Dict[str, datetime.datetime]:
    """Returns the "Merged-until: <repo> <time>" trailers of a message."""
    result = {}
    for line in message.splitlines():
        if line.startswith(MERGED_UNTIL):
            fields = line[len(MERGED_UNTIL):].split()
            if len(fields) == 2:
                result[fields[0]] = to_utc(fields[1])
    return result


def last_collection(ref: str, changelog_path: str,
                    repos: Sequence[str]) -> Dict[str, datetime.datetime]:
    """Returns, for each repo, the time of its last collection on ref.

    Walks the collection commits (subject starting with COLLECT_PREFIX) from
    the newest, and takes for each repo its newest "Merged-until:" trailer;
    Natron's must be in the newest collection commit.
    A repo of --extra-repo that no trailer mentions (its first collection)
    starts at the committer date of the newest collection commit.

    Raises:
        SystemExit: no collection commit, or the newest one has no trailer
            for the main repository (e.g. squashed): --since is needed.
    """
    log = _run(['git', 'log', '--format=%x01%H%x00%cI%x00%s%x00%B',
                f'--grep=^{COLLECT_PREFIX}', ref, '--', changelog_path])
    commits = [record.split('\x00', 3) for record in log.split('\x01')[1:]]
    commits = [c for c in commits if c[2].startswith(COLLECT_PREFIX)]
    if not commits:
        raise SystemExit(
            f'error: no "{COLLECT_PREFIX}" commit changed {changelog_path} '
            f'on {ref}; give the starting point with --since')
    newest = parse_merged_until(commits[0][3])
    if MAIN_REPO in repos and MAIN_REPO not in newest:
        raise SystemExit(
            f'error: the last "{COLLECT_PREFIX}" commit on {ref} '
            f'({commits[0][0][:10]}) has no "{MERGED_UNTIL} {MAIN_REPO}" '
            'trailer (squashed?); give the starting point with --since')
    until: Dict[str, datetime.datetime] = {}
    for _, _, _, message in commits:
        for repo, when in parse_merged_until(message).items():
            until.setdefault(repo, when)
    sha, date = commits[0][0], commits[0][1]
    for repo in repos:
        if repo not in until:
            print(f'note: no "{MERGED_UNTIL} {repo}" trailer: starting at '
                  f'the date of {sha[:10]}', file=sys.stderr)
            until[repo] = to_utc(date)
    return {repo: until[repo] for repo in repos}


def resolve_since(since: Optional[str], ref: str, changelog_path: str,
                  repos: Sequence[str]) -> Dict[str, datetime.datetime]:
    """Turns --since (a date, a timestamp or a commit) into per-repo times."""
    if since is None:
        return last_collection(ref, changelog_path, repos)
    commit = subprocess.run(
        ['git', 'log', '-1', '--format=%cI', f'{since}^{{commit}}', '--'],
        capture_output=True, text=True, check=False)
    if commit.returncode == 0:
        when = to_utc(commit.stdout.strip())
    else:
        try:
            when = to_utc(since)
        except ValueError:
            raise SystemExit(f'error: --since {since}: not a commit or a '
                             'date') from None
    return {repo: when for repo in repos}


def fetch_merged_prs(repo: str, base: str, since: datetime.datetime,
                     limit: int) -> List[PullRequest]:
    """Returns the PRs of repo merged into base after since, in merge order.

    Uses a single `gh pr list` search, which gh paginates.
    """
    stamp = since.strftime('%Y-%m-%dT%H:%M:%SZ')
    raw = json.loads(_run([
        'gh', 'pr', 'list', '-R', repo, '--state', 'merged', '--base', base,
        '--search', f'merged:>={stamp}', '--limit', str(limit),
        '--json', 'number,body,mergedAt,labels']))
    if len(raw) >= limit:
        print(f'warning: {repo}: got {limit} PRs, the --limit; some may be '
              'missing', file=sys.stderr)
    prs = [PullRequest(repo, p['number'], p['body'] or '', p['mergedAt'],
                       tuple(label['name'] for label in p['labels']))
           for p in raw if to_utc(p['mergedAt']) > since]
    return sorted(prs, key=lambda p: to_utc(p.merged_at))


def commit_message(prs: Sequence[PullRequest],
                   since: Dict[str, datetime.datetime]) -> str:
    """Returns the message of the collection commit."""
    main = [p for p in prs if p.repo == MAIN_REPO] or list(prs)
    if main:
        lines = [f'{COLLECT_PREFIX} {main[0].label}..{main[-1].label}', '',
                 'Lines from the descriptions of the merged PRs:',
                 ' '.join(p.label for p in prs), '']
    else:
        lines = [f'{COLLECT_PREFIX}: no new PR, regroup the entries', '']
    for repo in since:
        merged = [p.merged_at for p in prs if p.repo == repo]
        if merged:
            lines.append(f'{MERGED_UNTIL} {repo} {merged[-1]}')
        else:
            stamp = since[repo].strftime('%Y-%m-%dT%H:%M:%SZ')
            lines.append(f'{MERGED_UNTIL} {repo} {stamp}')
    return '\n'.join(lines) + '\n'


def _repo_and_branch(value: str) -> Tuple[str, str]:
    repo, _, branch = value.partition(':')
    if repo.count('/') != 1:
        raise argparse.ArgumentTypeError(f'not OWNER/NAME[:BRANCH]: {value}')
    return repo, branch or 'master'


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    """Parses the command line."""
    parser = argparse.ArgumentParser(
        description='Collect the "### CHANGELOG" sections of the PRs merged '
        'since the last collection into CHANGELOG.md.',
        epilog='Without --write or --commit, prints the diff (--dry-run). '
        'Problems (PRs without a CHANGELOG section, unknown headings) go to '
        'the standard error.')
    parser.add_argument('--repo', default=MAIN_REPO,
                        help='GitHub repository (default: %(default)s)')
    parser.add_argument('--base', default='RB-2.6',
                        help='base branch of the PRs (default: %(default)s)')
    parser.add_argument(
        '--extra-repo', action='append', default=[], type=_repo_and_branch,
        metavar='OWNER/NAME[:BRANCH]',
        help='also collect the PRs merged into BRANCH (default: master) of '
        'this repository, into the Plugins section; repeatable, e.g. '
        '--extra-repo NatronGitHub/openfx-io')
    parser.add_argument(
        '--since', metavar='SHA|DATE',
        help='collect the PRs merged after this commit (its committer date) '
        'or time (YYYY-MM-DD or ISO 8601, UTC without an offset); default: '
        f'the last "{COLLECT_PREFIX}" commit of the base branch')
    parser.add_argument(
        '--internal-label', action='append', default=list(INTERNAL_LABELS),
        metavar='LABEL',
        help='drop the lines of the PRs with this label; repeatable '
        '(default: %(default)s)')
    parser.add_argument('--include-internal', action='store_true',
                        help='keep the lines of the PRs with an internal '
                        'label')
    parser.add_argument('--changelog', default='CHANGELOG.md',
                        help='path of the CHANGELOG (default: %(default)s)')
    parser.add_argument('--limit', type=int, default=1000,
                        help='maximum number of PRs per repository '
                        '(default: %(default)s)')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--dry-run', action='store_const', dest='mode',
                      const='dry-run', help='print the diff (the default)')
    mode.add_argument('--write', action='store_const', dest='mode',
                      const='write', help='edit the CHANGELOG, and save the '
                      'commit message for --commit')
    mode.add_argument('--commit', action='store_const', dest='mode',
                      const='commit', help='commit the CHANGELOG as it is '
                      '(after --write and the editorial pass) with the '
                      f'message saved by --write, "{COLLECT_PREFIX} #a..#b"; '
                      'nothing is fetched, nothing is pushed')
    parser.add_argument('--force', action='store_true',
                        help='--write even if the CHANGELOG has uncommitted '
                        'changes')
    parser.set_defaults(mode='dry-run')
    return parser.parse_args(argv)


def _message_path() -> str:
    """Returns where --write saves the commit message for --commit."""
    return _run(['git', 'rev-parse', '--git-path',
                 'COLLECT_CHANGELOG_MSG']).strip()


def _commit(changelog_path: str) -> int:
    """Commits the CHANGELOG with the message saved by --write."""
    path = _message_path()
    if not os.path.exists(path):
        raise SystemExit(f'error: no {path}: run with --write first')
    subprocess.run(['git', 'commit', '-q', '-F', path, '--',
                    changelog_path], check=True)
    os.remove(path)
    print(_run(['git', 'log', '-1', '--format=%h %s']), end='',
          file=sys.stderr)
    return 0


def main(argv: Optional[Sequence[str]] = None) -> int:
    """Runs the collector; returns the exit status."""
    args = parse_args(sys.argv[1:] if argv is None else argv)
    if args.mode == 'commit':
        return _commit(args.changelog)
    if args.mode == 'write' and not args.force and subprocess.run(
            ['git', 'diff', '--quiet', 'HEAD', '--', args.changelog],
            check=False).returncode != 0:
        raise SystemExit(
            f'error: {args.changelog} has uncommitted changes (an earlier '
            '--write, or the editorial pass): a new --write would add the '
            'lines again. Commit or restore it first, or use --force.')
    branches = dict([(args.repo, args.base)] + args.extra_repo)
    since = resolve_since(args.since, _git_ref(args.base), args.changelog,
                          list(branches))
    prs = []
    for repo, branch in branches.items():
        found = fetch_merged_prs(repo, branch, since[repo], args.limit)
        print(f'{repo}: {len(found)} PR(s) merged into {branch} after '
              f'{since[repo].isoformat()}: '
              + ' '.join(p.label for p in found), file=sys.stderr)
        prs.extend(found)
    prs.sort(key=lambda p: to_utc(p.merged_at))
    with open(args.changelog, encoding='utf-8') as f:
        old = f.read()
    new, problems = collect(prs, old,
                            () if args.include_internal
                            else args.internal_label)
    for problem in problems:
        print(f'{problem.pr}: {problem.message}', file=sys.stderr)
    if args.mode == 'dry-run':
        sys.stdout.writelines(difflib.unified_diff(
            old.splitlines(keepends=True), new.splitlines(keepends=True),
            f'a/{args.changelog}', f'b/{args.changelog}'))
        return 0
    with open(args.changelog, 'w', encoding='utf-8') as f:
        f.write(new)
    with open(_message_path(), 'w', encoding='utf-8') as f:
        f.write(commit_message(prs, since))
    print(f'wrote {args.changelog}: rewrite the topmost version for users '
          '(see the maintainer guide), then run with --commit',
          file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
