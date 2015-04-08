#! /usr/bin/env python
from subprocess import Popen, PIPE
import re


def getTags():
    Popen('git fetch --tags'.split(), stdout=PIPE).communicate()
    (stdout, _) = Popen('git tag'.split(), stdout=PIPE).communicate()

    return sorted(stdout.split(), key=lambda s: [int(x) for x in s.replace('v', '').split('.')])


def checkLastChangelogTag():
    last = None
    with open('CHANGELOG.md', 'r+') as f:
        lines = f.readlines()

        for line in lines:
            m = re.search(r'^##\s+(\S+):', line)
            if m:
                last = m.group(1)
                break

    return last


def buildNewLogs(fromTag, toTag):
    stdout = ''
    if fromTag:
        (stdout, _) = Popen(('git rev-list %s..%s' % (fromTag, toTag)).split(), stdout=PIPE).communicate()
    else:
        (stdout, _) = Popen(('git rev-list %s' % toTag).split(), stdout=PIPE).communicate()

    commits = stdout.splitlines()
    feats = []
    fixs = []
    brokens = []

    for commit in commits:
        (title, _) = Popen(('git show -s --format=%%s %s' % commit).split(), stdout=PIPE).communicate()
        (body, _) = Popen(('git show -s --format=%%b %s' % commit).split(), stdout=PIPE).communicate()
        if not title:
            continue

        data = title.split(' ', 1)

        if data[0] == 'feat':
            feats.append(data[1].rstrip())
        elif data[0] == 'fix':
            fixs.append(data[1].rstrip())

        if 'BROKEN:' in body:
            brokens += body.split('BROKEN:')[1].splitlines()

    logs = "## %s:\n" % toTag

    if not len(feats) and not len(fixs) and not len(brokens):
        logs += "*No major changes.*\n\n\n"
    else:
        if len(feats):
            logs += "\n#### New features:\n"
            for feat in feats:
                logs += " - %s\n" % feat
        if len(fixs):
            logs += "\n#### Bug fixes:\n"
            for fix in fixs:
                logs += " - %s\n" % fix
        if len(brokens):
            logs += "\n#### Breaking changes:\n"
            for broken in brokens:
                if broken.rstrip() != '':
                    logs += " - %s\n" % broken
        logs += "\n\n"

    return logs


if __name__ == "__main__":
    tags = getTags()
    lastChangelogTag = checkLastChangelogTag()

    changelog = ''
    tagsToBuild = tags
    previousTag = None
    if lastChangelogTag:
        previousTag = lastChangelogTag
        tagsToBuild = tags[tags.index(lastChangelogTag) + 1:]
    else:
        tagsToBuild = tags[1:] # ignoring first release which contains only the first commit

    with open('CHANGELOG.md', 'r+') as f:
        changelog = f.read().replace('# Changelog\n\n', '').rstrip() + '\n'

    if not len(tagsToBuild):
        print "No new changlogs! Last tag (%s) is already in the CHANGELOG.md." % lastChangelogTag
        exit(0)

    for tag in tagsToBuild:
        newLogs = buildNewLogs(previousTag, tag)
        previousTag = tag
        changelog = newLogs + changelog

    changelog = '# Changelog\n\n' + changelog

    with open('CHANGELOG.md', 'w') as f:
        f.write(changelog)
