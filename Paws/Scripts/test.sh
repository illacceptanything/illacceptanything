#!/usr/bin/env sh

test_env="$npm_package_config_test_ENV"
test_files="$npm_package_config_test_files"
mocha_ui="$npm_package_config_mocha_ui"
mocha_reporter="$npm_package_config_mocha_reporter"

if [ "$PRE_COMMIT" = "true" ]; then
   mocha_reporter='dot'
   RESPECT_TRACING='no'
fi

if [ -n "${RESPECT_TRACING##[NFnf]*}" ]; then
   trace_reactor='no'
   verbose='4'          # 'warning' and worse
fi

[ -z "${SILENT##[NFnf]*}${QUIET##[NFnf]*}" ] && [ "${VERBOSE:-4}" -gt 6 ] && print_commands=yes
go () { [ -z ${print_commands+x} ] || puts '`` '"$*" >&2 ; "$@" || exit $? ;}

go env TRACE_REACTOR="$trace_reactor" VERBOSE="$verbose" \
    NODE_ENV="$test_env" ./node_modules/.bin/mocha       \
   --compilers coffee:coffee-script/register             \
   --reporter "$mocha_reporter" --ui "$mocha_ui"         \
   $MOCHA_FLAGS "$test_files"

# FIXME: Check if the directories exist, but are empty.
if [ -d "$PWD/$npm_package_config_dirs_rulebook" ]; then
   if [ -d "$PWD/$npm_package_config_dirs_rulebook/The Ladder/" ]; then
      go env TRACE_REACTOR="$trace_reactor" VERBOSE="$verbose"    \
          NODE_ENV="$test_env" ./node_modules/.bin/taper          \
         --runner "$PWD/Executables/paws.js"                      \
         --runner-param='check'                                   \
         "$PWD/$npm_package_config_dirs_rulebook/The Ladder/"*    \
         $TAPER_FLAGS -- $CHECK_FLAGS
   fi
   
   if [ -d "$PWD/$npm_package_config_dirs_rulebook/The Gauntlet/" ]; then
      go env TRACE_REACTOR="$trace_reactor" VERBOSE="$verbose"    \
          NODE_ENV="$test_env" ./node_modules/.bin/taper          \
         --runner "$PWD/Executables/paws.js"                      \
         --runner-param='check'                                   \
         "$PWD/$npm_package_config_dirs_rulebook/The Gauntlet/"*  \
         $TAPER_FLAGS -- $CHECK_FLAGS
   fi
   
   if [ -n "${RUN_LETTERS##[NFnf]*}" ] && \
      [ -d "$PWD/$npm_package_config_dirs_rulebook/The Letters/" ]; then
      go env TRACE_REACTOR="$trace_reactor" VERBOSE="$verbose"    \
          NODE_ENV="$test_env" ./node_modules/.bin/taper          \
         --runner "$PWD/Executables/paws.js"                      \
         --runner-param='check'                                   \
         --runner-param='--expose-specification'                  \
         "$PWD/$npm_package_config_dirs_rulebook/The Letters/"*   \
         $TAPER_FLAGS -- $CHECK_FLAGS $RULEBOOK_FLAGS
   fi
fi
