2.1.1 / 2015-01-09
==================

 * Smoother rendering for progress bar
 * Fixed progress bar for case where no examples are found
 * Tidier output alignment + block width
 * Removed deprecated calls to Yaml::parse
 * More accurate lower bounds for composer installation

2.1.0 / 2014-12-14
==================

 * No changes from RC3

2.1.0-RC3 / 2014-12-04
======================

 * Removed minor BC break introduced in RC2

2.1.0-RC2 / 2014-11-14
======================

  * Specify bootstrap file via configuration
  * Correct error codes while using --stop-on-failure
  * Better detection of empty specs
  * Fixed issue where non-spec files in spec folder caused errors
  * Better PSR-4 support

2.1.0-RC1 / 2014-09-14
======================

  * Allow objects to be instantiated via static factory methods
  * Automatic generation of return statements using '--fake'
  * Test suite is automatically rerun when classes or methods have been generated
  * Allow examples to mark themselves as skipped
  * PSR-4 support
  * PSR-0 locator now supports underscores correctly
  * Ability to specify a custom bootstrap file using '--bootstrap' (for autoloader registration etc)
  * Ability to have a personal .phpspec.yml in home folder
  * Progress bar grows from left to right and flickers less
  * Improved diffs for object comparison
  * Throw an exception when construction method is redefined
  * Non-zero exit code when dependencies are missing
  * Respect exit code of commands other than 'run'
  * Higher CLI verbosity levels are handled properly
  * Code Generation and Stop on Failure are configurable through phpspec.yml
  * Fixes for object instantiation changes in newer versions of PHP
  * PHP 5.6 support
  * Fixes for progress bar sometimes rounding up to 100% when not all specs passed
  * Support for non-standard Composer autoloader location
  * Improved hhvm support
  * Extensions can now register new command
  * Resource locator de-duplicates resources (supports custom locators in extensions)

2.0.1 / 2014-07-01
==================

  * Fixed the loading of the autoloader for projects using a custom composer vendor folder

2.0.0 / 2014-03-19
==================

  * Improve support to windows
  * Improve support to hhvm
  * Improve acceptance tests coverage with Behat

2.0.0-RC4 / 2014-02-21
======================

  * Revamped junit formatter
  * Fixed #269 Problem with exception masking and generation for not found class
  * HHVM is officially supported
  * Add psr0 validator
  * Remove Nyan from core
  * Added an exception if the specified config file does not exist
  * Fixed a problem with generating a constructor when it is first time added
  * Improved help
  * Fixed the suite runner in fast machines

2.0.0-RC3 / 2014-01-01
======================

  * Fixed the Prophecy constraint as the new release is 1.1
  * Refactored formatters to be defined as services

2.0.0-RC2 / 2013-12-30
======================

  * Fixed the invocation of methods expecting an argument passed by reference
  * Fixed the instantiation of the wrapped object in shouldThrow

2.0.0-RC1 / 2013-12-26
======================

  * Bump the Prophecy requirement to ``~1.0.5@dev``
  * Added a JUnit formatter
  * Added the ``--stop-on-failure`` option
  * Fixed the support of the ``--no-interaction`` option
  * Added more events to add extension points
  * Added the number of specs in the console output
  * Fixed the handling of Windows line endings in the StringEngine and in reading doc comments
  * Added extension points in the template loading
  * Added a constructor generator
  * Added a HTML formatter
  * Added a nyan cat formatter

2.0.0beta4 / 2013-05-19
=======================

  * Add collaborator constructor setter
  * Fix couple of bugs in Prophecy integration layer
  * New (old) dot formatter

2.0.0beta3 / 2013-05-01
=======================

  * Prevent loading of unexisting PHP files
  * Fix typos in the error messages

2.0.0beta2 / 2013-04-30
=======================

  * Bump required Prophecy version to 1.0.1
  * Support non-string values with ArrayContain matcher
  * Create `src` folder if does not exist
  * Fix stack trace and matchers failure printing

2.0.0beta1 / 2013-04-29
=======================

  * Initial release

