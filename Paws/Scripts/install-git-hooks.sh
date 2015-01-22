#!/usr/bin/env sh
                                                                                      set +o verbose
# Usage:
# ------
# This script will convert your local .git/hooks directory to allow for multiple scripts for each
# `git`-hook, and will then install the hooks associated with contributing to this project.
# 
#     VERBOSE=true npm run-script install-git-hooks
puts() { printf %s\\n "$*" ;}

hook_dir=".git/hooks"
tracked_dir="Scripts/git-hooks"
requested_hook="$1"

# ‘Arrays’ in POSIX shell are a nasty topic. It's possible to work relatively sanely with
# newline-delimited strings, but since git hooks' names contain no spaces, I'll simply use a
# space-delimited string for this.  /=
default_hooks="$npm_package_config_git_hooks"


# FIXME: This should really support comma-seperated DEBUG values, as per `node-debug`:
#        https://github.com/visionmedia/debug
[ "$DEBUG" = 'Paws.js:scripts' ] && DEBUG_SCRIPTS=0
[ -n "$DEBUG_SCRIPTS" ] && puts "Script debugging enabled (in: `basename $0`)." >&2
[ -n "$DEBUG_SCRIPTS" ] && VERBOSE=0

[ -n "$DEBUG_SCRIPTS" ] && puts \
   "Requested hook:        '$1'"                               \
   "Default hooks:         '$npm_package_config_git_hooks'"    \
   "" >&2

if [ ! -d ".git" ]; then
   puts 'You must be in the root directory of a `git` project to use this script!' >&2
   exit 1                                                                                        ;fi

mkdir -p "$hook_dir"


install_hook() {
   hook_name="$1"
   
   [ -n "$VERBOSE" ] && puts "Installing '$hook_name' hooks ..." >&2
   
   # If the primary hook-file exists, and isn't a symlink, then we need to move it out of the way.
   if [ ! -h "$hook_dir/$hook_name" -a -x "$hook_dir/$hook_name" ]; then
      puts "Moving original '$hook_name' to '$hook_name-local' ..." >&2
      mv "$hook_dir/$hook_name" "$hook_dir/$hook_name-local" || exit 1                           ;fi
   
   # If it still exists now, it's either already a symlink or not executable. Either way, don't care
   rm "$hook_dir/$hook_name" 2>/dev/null
   
   # Now, we link the hook-chaining script to process these hooks
   ln -s "../../$tracked_dir/chain-hooks.sh" "$hook_dir/$hook_name"
   
   # For each tracked hook in the repository, 
   for tracked_path in "$tracked_dir/$hook_name"-*; do
      if [ ! -x "$tracked_path" ]; then continue                                                 ;fi
      
      hook_path="$hook_dir/$(basename "$tracked_path")"
      [ -n "$VERBOSE" ] && puts " - '$hook_path' ..." >&2
      
      # ... remove any existing symlink,
      if [ -L "$hook_path" ]; then
         rm $hook_path                                                                           ;fi
      
      # ... and create a symlink to the tracked version of the hook.
      ln -s "../../$tracked_path" "$hook_path" || exit 1
      
   done
}

if [ -n "$requested_hook" ]; then 
   install_hook $requested_hook
else for default_hook in $default_hooks; do
   install_hook $default_hook          ; done                                                    ;fi
