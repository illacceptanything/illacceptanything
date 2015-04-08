Feature: Developer runs the specs with bootstrap option
  As a Developer
  I want to run the specs and specify bootstrap file
  In order to get feedback on a state of my application

  Scenario: Running a spec with --bootstrap option
    Given the bootstrap file "bootstrap.php" contains:
      """
      <?php
      throw new \Exception('bootstrap file is loaded');
      """
    When I run phpspec with option --bootstrap=bootstrap.php
    Then I should see "bootstrap file is loaded"

  Scenario: Running a spec with bootstrap option in config file
    Given the bootstrap file "bootstrap.php" contains:
      """
      <?php
      throw new \Exception('bootstrap file is loaded');
      """
    And the config file contains:
      """
      bootstrap: bootstrap.php
      """
    When I run phpspec
    Then I should see "bootstrap file is loaded"

  Scenario: Running a spec with --bootstrap option and bootstrap file is missing.
    Given there is no file "missing.php"
    When I run phpspec with option --bootstrap=missing.php
    Then I should see "Bootstrap file 'missing.php' does not exist"
