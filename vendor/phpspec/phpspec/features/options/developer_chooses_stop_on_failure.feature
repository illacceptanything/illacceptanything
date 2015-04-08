Feature: Developer chooses stop on failure
  As a Developer
  I want to set the stop on failure setting option
  In order to specify how phpspec behaves on failure

  @issue352
  Scenario: stop-on-failure defaults to off
    Given the spec file "spec/SkipOnFailure/SpecExample1/FirstFailSpec.php" contains:
      """
      <?php

      namespace spec\SkipOnFailure\SpecExample1;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class FirstFailSpec extends ObjectBehavior
      {
          function it_fails()
          {
              $this->getValue()->shouldReturn(2);
          }

          function it_should_never_get_called()
          {
              $this->getValue()->shouldReturn(1);
          }
      }

      """
    And the class file "src/SkipOnFailure/SpecExample1/FirstFail.php" contains:
      """
      <?php

      namespace SkipOnFailure\SpecExample1;

      class FirstFail
      {
          public function getValue()
          {
              return 1;
          }
      }

      """
    When I run phpspec
    Then 2 examples should have been run

  @issue352
  Scenario: stop-on-failure is specified in the config
    Given the config file contains:
      """
      stop_on_failure: true
      """
    And the spec file "spec/SkipOnFailure/SpecExample2/FirstFailSpec.php" contains:
      """
      <?php

      namespace spec\SkipOnFailure\SpecExample2;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class FirstFailSpec extends ObjectBehavior
      {
          function it_fails()
          {
              $this->getValue()->shouldReturn(2);
          }

          function it_should_never_get_called()
          {
              $this->getValue()->shouldReturn(1);
          }
      }

      """
    And the class file "src/SkipOnFailure/SpecExample2/FirstFail.php" contains:
      """
      <?php

      namespace SkipOnFailure\SpecExample2;

      class FirstFail
      {
          public function getValue()
          {
              return 1;
          }
      }

      """
    When I run phpspec
    Then 1 example should have been run
    And the exit code should be 1

  @issue352
  Scenario: stop-on-failure at command line overrides config
    Given the config file contains:
      """
      stop_on_failure: false
      """
    And the spec file "spec/SkipOnFailure/SpecExample3/FirstFailSpec.php" contains:
      """
      <?php

      namespace spec\SkipOnFailure\SpecExample3;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class FirstFailSpec extends ObjectBehavior
      {
          function it_fails()
          {
              $this->getValue()->shouldReturn(2);
          }

          function it_should_never_get_called()
          {
              $this->getValue()->shouldReturn(1);
          }
      }

      """
    And the class file "src/SkipOnFailure/SpecExample3/FirstFail.php" contains:
      """
      <?php

      namespace SkipOnFailure\SpecExample3;

      class FirstFail
      {
          public function getValue()
          {
              return 1;
          }
      }

      """
    When I run phpspec with the "stop-on-failure" option
    Then 1 example should have been run
    And the exit code should be 1

