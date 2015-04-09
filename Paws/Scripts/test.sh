#!/usr/bin/env sh
                                                                                      set +o verbose
# Usage:
# ------
# This script runs both the unit-test suite and, if they've been downloaded, checks conformance with
# the Paws rulebooks.
# 
#    npm test
#    MOCHA_FLAGS='--grep Parser' nrs test # Run a specific test-suite
#    RESPECT_TRACING=no npm test          # Disable debugging and tracing during the tests
#    RUN_RULEBOOK=no npm test             # Ignore the Rulebook, even if present
#    RUN_LETTERS=yes npm test             # To execute the Letters, as well
puts() { printf %s\\n "$@" ;}
pute() { printf %s\\n "~~ $*" >&2 ;}

test_env="$npm_package_config_test_ENV"
test_files="$npm_package_config_test_files"
mocha_ui="$npm_package_config_mocha_ui"
mocha_reporter="$npm_package_config_mocha_reporter"

# FIXME: This should support *excluded* modules with a minus, as per `node-debug`:
#        https://github.com/visionmedia/debug
echo "$DEBUG" | grep -qE '(^|,\s*)(\*|Paws.js(:(scripts|\*))?)($|,)' && DEBUG_SCRIPTS=0
[ -n "$DEBUG_SCRIPTS" ] && pute "Script debugging enabled (in: `basename $0`)."
[ -n "$DEBUG_SCRIPTS" ] && VERBOSE="${VERBOSE:-7}"

if [ -n "${PRE_COMMIT##[NFnf]*}" ]; then
   [ -n "$DEBUG_SCRIPTS" ] && pute "Enabling pre-commit mode."
   mocha_reporter='dot'
   RESPECT_TRACING='no'
fi

if [ -n "${RESPECT_TRACING##[YTyt]*}" ]; then
   [ -n "$DEBUG_SCRIPTS" ] && pute "Disrespecting tracing flags"
   VERBOSE='4'          # 'warning' and worse
   unset TRACE_REACTOR
fi

[ -z "${SILENT##[NFnf]*}${QUIET##[NFnf]*}" ] && [ "${VERBOSE:-4}" -gt 6 ] && print_commands=yes
go () { [ -z ${print_commands+x} ] || puts '`` '"$*" >&2 ; "$@" || exit $? ;}

[ -n "$DEBUG_SCRIPTS" ] && puts \
   "Rulebook directory:    '$npm_package_config_dirs_rulebook'"   \
   "Running letters:       ${RUN_LETTERS:--}"                     \
   "Pre-commit mode:       ${PRE_COMMIT:--}"                      \
   "Verbosity:             '$VERBOSE'"                            \
   "Tracing reactor:       ${TRACE_REACTOR:+Yes!}"                \
   "Printing commands:     ${print_commands:--}"                  \
   "" >&2


go env NODE_ENV="$test_env" ./node_modules/.bin/mocha    \
   --compilers coffee:coffee-script/register             \
   --reporter "$mocha_reporter" --ui "$mocha_ui"         \
   $MOCHA_FLAGS "$test_files"

# FIXME: Check if the directories exist, but are empty.
if [ -d "$PWD/$npm_package_config_dirs_rulebook" ]; then
   if [ -n "${RUN_RULEBOOK##[YTyt]*}" ]; then exit 0; fi

   if [ -d "$PWD/$npm_package_config_dirs_rulebook/The Ladder/" ]; then
      go env NODE_ENV="$test_env" ./node_modules/.bin/taper       \
         --runner "$PWD/Executables/paws.js"                      \
         --runner-param='check'                                   \
         "$PWD/$npm_package_config_dirs_rulebook/The Ladder/"*    \
         $TAPER_FLAGS -- $CHECK_FLAGS
   fi
   
   if [ -d "$PWD/$npm_package_config_dirs_rulebook/The Gauntlet/" ]; then
      go env NODE_ENV="$test_env" ./node_modules/.bin/taper       \
         --runner "$PWD/Executables/paws.js"                      \
         --runner-param='check'                                   \
         "$PWD/$npm_package_config_dirs_rulebook/The Gauntlet/"*  \
         $TAPER_FLAGS -- $CHECK_FLAGS
   fi
   
   if [ -n "${RUN_LETTERS##[NFnf]*}" ] && \
      [ -d "$PWD/$npm_package_config_dirs_rulebook/The Letters/" ]; then
      go env NODE_ENV="$test_env" ./node_modules/.bin/taper       \
         --runner "$PWD/Executables/paws.js"                      \
         --runner-param='check'                                   \
         --runner-param='--expose-specification'                  \
         "$PWD/$npm_package_config_dirs_rulebook/The Letters/"*   \
         $TAPER_FLAGS -- $CHECK_FLAGS $RULEBOOK_FLAGS
   fi
   
else
   [ -n "$DEBUG_SCRIPTS" ] && pute "Rulebook directory not found."

   puts 'Clone the rulebook from this URL to `./'$npm_package_config_dirs_rulebook'` to check Rulebook compliance:'
   puts '   <https://github.com/Paws/Rulebook.git>'
fi
