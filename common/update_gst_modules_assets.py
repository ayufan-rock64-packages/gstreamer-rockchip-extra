#!/usr/bin/env python3

import os
import shutil
import sys
import argparse
import subprocess

parser = argparse.ArgumentParser(prog="update-gstreamer-common-files",
                                 description="Copy files that are common to several repository"
                                 " to the right places in those.")
parser.add_argument("-m", "--commit-message",
                    help="git add modified files and commit the changes with this message.")
parser.add_argument('rootdir', metavar='rootdir', nargs=1,
                    help='The directory where to find GStreamer modules repoos.')


SCRIPTDIR = os.path.dirname(__file__)
FILES_TO_COPY = ['hooks/pre-commit.hook']
GETPLUGINSDIR = 'tests/check/getpluginsdir'

FILES_TO_COPY_PER_MODULE = {
    'gstreamer': [],
    'gst-plugins-base': [GETPLUGINSDIR],
    'gst-plugins-good': [GETPLUGINSDIR],
    'gst-plugins-ugly': [GETPLUGINSDIR],
    'gst-plugins-bad': [],
    'gst-libav': [],
    'gst-editing-services': [GETPLUGINSDIR],
    'gst-devtools': [(GETPLUGINSDIR, "validate/tests/")],
    'gst-python': [],
}

if __name__ == "__main__":
    options = parser.parse_args()
    rootpath = options.rootdir[0]
    if not os.path.exists(rootpath):
        print("Please path the root directory where to find GStreamer modules repos"
              " %s does not exists", rootpath)
        exit(1)

    summary = ""
    for module, files in FILES_TO_COPY_PER_MODULE.items():
        files += FILES_TO_COPY

        repodir = os.path.abspath(os.path.join(rootpath, module))
        if not os.path.exists(repodir):
            print("Repo %s does not exists" % repodir)
            exit(1)

        if options.commit_message:
            dirty = False
            try:
                dirty = subprocess.check_output('git status --porcelain --untracked-files=no'.split(' '),
                                cwd=repodir)
            except Exception as e:
                dirty = True

            if dirty:
                print("Can not commit %s as the repository is dirty." %
                        (repodir))
                exit(1)

        for f in files:
            if isinstance(f, tuple):
                destdir = os.path.join(repodir, f[1])
                f = f[0]
            else:
                dirname = os.path.dirname(f)
                destdir = os.path.join(repodir, dirname)

            if destdir and not os.path.exists(destdir):
                print("Making dir %s" % destdir)
                os.makedirs(destdir, exist_ok=True)

            dest = os.path.join(destdir, os.path.basename(f))
            print("Copying %s to %s" %(f, dest))
            shutil.copy(os.path.join(SCRIPTDIR, f), dest)
            if options.commit_message:
                subprocess.check_call(['git', 'add', os.path.abspath(dest)],
                                      cwd=repodir)

        if options.commit_message:
            if subprocess.call('git diff-index --quiet --cached HEAD'.split(' '), cwd=repodir) != 0:
                subprocess.call(['git', 'commit', '-m', options.commit_message], cwd=repodir)
                if not summary:
                    summary = "Commited:\n"
                summary += repodir + '\n'

    print(summary)
