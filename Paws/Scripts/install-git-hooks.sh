#!/usr/bin/env sh

# Usage:
# ------
# This script will convert your local .git/hooks directory to allow for multiple scripts for each
# `git`-hook, and will then install the hooks associated with contributing to this project.
# 
#     VERBOSE=true npm run-script install-git-hooks

hook_dir=".git/hooks"
tracked_dir="Scripts/git-hooks"
requested_hook="$1"

# ‘Arrays’ in POSIX shell are a nasty topic. It's possible to work relatively sanely with
# newline-delimited strings, but since git hooks' names contain no spaces, I'll simply use a
# space-delimited string for this.  /=
default_hooks="pre-commit post-merge post-checkout"

if [ ! -d "$hook_dir" ]; then
   printf %s\\n 'You must be in the root directory of a `git` project to use this script!' >&2
   exit 1                                                                                       ;fi


install_hook() {
   hook_name="$1"
   
   if [ -n "$VERBOSE" ]; then
      printf %s\\n 'Installing "'"$hook_name"'" hooks ...' >&2                                  ;fi
   
   # If the primary hook-file exists, and isn't a symlink, then we need to move it out of the way.
   if [ ! -h "$hook_dir/$hook_name" -a -x "$hook_dir/$hook_name" ]; then
      printf %s\\n 'Moving original "'"$hook_name"'" to "'"$hook_name"'-local" ...' >&2
      mv "$hook_dir/$hook_name" "$hook_dir/$hook_name"'-local' || exit 1                        ;fi
   
   # If it still exists now, it's either already a symlink or not executable. Either way, don't care
   rm "$hook_dir/$hook_name" 2>/dev/null
   
   # Now, we link the hook-chaining script to process these hooks
   ln -s "../../$tracked_dir/chain-hooks.sh" "$hook_dir/$hook_name"
   
   # For each tracked hook in the repository, 
   for tracked_path in "$tracked_dir/$hook_name"-*; do
      if [ ! -x "$tracked_path" ]; then continue                                                ;fi
      
      hook_path="$hook_dir/$(basename "$tracked_path")"
      if [ -n "$VERBOSE" ]; then
         printf %s\\n ' - "'"$hook_path"'" ...' >&2                                             ;fi
      
      # ... remove any existing symlink,
      if [ -L "$hook_path" ]; then
         rm $hook_path                                                                          ;fi
      
      # ... and create a symlink to the tracked version of the hook.
      ln -s "../../$tracked_path" "$hook_path" || exit 1
      
   done
}

if [ -n "$requested_hook" ]; then 
   install_hook $requested_hook
else for default_hook in $default_hooks; do
   install_hook $default_hook          ; done                                                   ;fi
