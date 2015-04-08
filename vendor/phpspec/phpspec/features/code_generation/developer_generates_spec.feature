Feature: Developer generates a spec
  As a Developer
  I want to automate creating specs
  In order to avoid repetitive tasks and interruptions in development flow

  Scenario: Generating a spec
    When I start describing the "CodeGeneration/SpecExample1/Markdown" class
    Then a new spec should be generated in the "spec/CodeGeneration/SpecExample1/MarkdownSpec.php":
      """
      <?php

      namespace spec\CodeGeneration\SpecExample1;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class MarkdownSpec extends ObjectBehavior
      {
          function it_is_initializable()
          {
              $this->shouldHaveType('CodeGeneration\SpecExample1\Markdown');
          }
      }

      """

  @issue127
  Scenario: Generating a spec with PSR0 must convert classname underscores to directory separator
    When I start describing the "CodeGeneration/SpecExample1/Text_Markdown" class
    Then a new spec should be generated in the "spec/CodeGeneration/SpecExample1/Text/MarkdownSpec.php":
      """
      <?php

      namespace spec\CodeGeneration\SpecExample1;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class Text_MarkdownSpec extends ObjectBehavior
      {
          function it_is_initializable()
          {
              $this->shouldHaveType('CodeGeneration\SpecExample1\Text_Markdown');
          }
      }

      """

  @issue127
  Scenario: Generating a spec with PSR0 must not convert namespace underscores to directory separator
    When I start describing the "CodeGeneration/Spec_Example2/Text_Markdown" class
    Then a new spec should be generated in the "spec/CodeGeneration/Spec_Example2/Text/MarkdownSpec.php":
      """
      <?php

      namespace spec\CodeGeneration\Spec_Example2;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class Text_MarkdownSpec extends ObjectBehavior
      {
          function it_is_initializable()
          {
              $this->shouldHaveType('CodeGeneration\Spec_Example2\Text_Markdown');
          }
      }

      """

  Scenario: Generating a spec for a class with psr4 prefix
    Given the config file contains:
      """
      suites:
        behat_suite:
          namespace: Behat\CodeGeneration
          psr4_prefix: Behat\CodeGeneration
      """
    When I start describing the "Behat/CodeGeneration/Markdown" class
    Then a new spec should be generated in the "spec/MarkdownSpec.php":
      """
      <?php

      namespace spec\Behat\CodeGeneration;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class MarkdownSpec extends ObjectBehavior
      {
          function it_is_initializable()
          {
              $this->shouldHaveType('Behat\CodeGeneration\Markdown');
          }
      }

      """