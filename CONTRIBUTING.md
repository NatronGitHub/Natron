Contributing to Natron
======================

Code contributions to Natron are always welcome. That's a big part of
why it's an open source project. Please review this document to get a
briefing on our process.

Code of Conduct
---------------

By contributing to Natron and the open source software projects managed
within the [Natron organization on GitHub](https://github.com/NatronGitHub),
you agree to follow the [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/version/1/4/code-of-conduct).

Examples of behavior that contributes to creating a positive environment
include:

* Using welcoming and inclusive language
* Being respectful of differing viewpoints and experiences
* Gracefully accepting constructive criticism
* Focusing on what is best for the community
* Showing empathy towards other community members

Examples of unacceptable behavior by participants include:

* The use of sexualized language or imagery and unwelcome sexual attention or
  advances
* Trolling, insulting/derogatory comments, and personal or political attacks
* Public or private harassment
* Publishing others' private information, such as a physical or electronic
  address, without explicit permission
* Other conduct which could reasonably be considered inappropriate in a
  professional setting


Bug Reports and Issue Tracking
------------------------------

We use GitHub's issue tracking system for bugs and enhancements:
https://github.com/NatronGitHub/Natron/issues

**If you are merely asking a question ("how do I...")**, please do not file an
issue, but instead ask the question on the [forum](https://discuss.pixls.us/c/software/natron).

If you are submitting a bug report, please be sure to note which version of
Natron you are using, and on what platform (OS/version).

Please give an account of

* what you tried
* what happened
* what you expected to happen instead

with enough detail that others can reproduce the problem.


Contributions Under Repository License
--------------------------------------

Whenever you make a contribution to a repository containing notice of a [license](https://github.com/NatronGitHub/Natron/blob/master/LICENSE.txt), you license your contribution under the same terms, and you agree that you have the right to license your contribution under those terms.

Pull Requests and Code Review
-----------------------------

The best way to submit changes is via GitHub Pull Request.

All code must be formally reviewed before being merged into the official repository. The protocol is like this:

1. Get a GitHub account, fork NatronGitHub/Natron to create your own repository on GitHub, and then clone it to get a repository on your local machine.

2. Edit, compile, and test your changes.

3. Push your changes to your fork (each unrelated pull request to a separate
"topic branch", please).

4. Make a "pull request" on GitHub for your patch, use the "master" branch.

5. If your patch will induce a major compatibility break, or has a design
component that deserves extended discussion or debate among the wider Natron
community, then it may be prudent to make a post on our [forum](https://discuss.pixls.us/c/software/natron) pointing everybody to
the pull request URL and discussing any issues you think are important.

6. The reviewer will look over the code and critique on the "comments" area,
or discuss in email/forum. Reviewers may ask for changes, explain problems they
found, congratulate the author on a clever solution, etc. Please don't take it hard if your
first try is not accepted. It happens to all of us.

7. After approval, one of the senior developers (with commit approval to the official main repository) will merge your fixes into the "master" branch.


Coding Style
------------

There are two overarching rules:

1. When making changes, conform to the style and conventions of the
surrounding code. Do not modify spacing, indentation, or symbol names
if there is no change to the underlying code.

2. Strive for clarity, even if that means occasionally breaking the
guidelines. Use your head and ask for advice if your common sense seems to
disagree with the conventions.
