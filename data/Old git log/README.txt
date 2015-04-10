On April 10, several months after this project was created and started accepting
pull requests, the repo's history was rewritten in order to permanently remove
some large files. This was done to make it easier for new contributors to clone
the repo (see https://github.com/illacceptanything/illacceptanything/issues/605)
. Unfortunately, this permanently removed all of the information that was
contained in the project's history.

I had a copy of the repo from the day before the history rewrite, so I ran
  git log > old_git_log.txt
to record all of the lost commit messages. That's what the 24MB text file in
this directory is. Enjoy! ;-)
