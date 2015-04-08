Feature: Developer generates a method
  As a Developer
  I want to automate creating methods
  In order to avoid repetitive tasks and interruptions in development flow

  Scenario: Generating a method
    Given the spec file "spec/CodeGeneration/MethodExample1/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\CodeGeneration\MethodExample1;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class MarkdownSpec extends ObjectBehavior
      {
          function it_converts_plain_text_to_html_paragraphs()
          {
              $this->toHtml('Hi, there')->shouldReturn('<p>Hi, there</p>');
          }
      }

      """
    And the class file "src/CodeGeneration/MethodExample1/Markdown.php" contains:
      """
      <?php

      namespace CodeGeneration\MethodExample1;

      class Markdown
      {
      }

      """
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then the class in "src/CodeGeneration/MethodExample1/Markdown.php" should contain:
      """
      <?php

      namespace CodeGeneration\MethodExample1;

      class Markdown
      {

          public function toHtml($argument1)
          {
              // TODO: write logic here
          }
      }

      """
  Scenario: Generating a method in a class with psr4 prefix
    Given the spec file "spec/MyNamespace/PrefixSpec.php" contains:
      """
      <?php

      namespace spec\Behat\Tests\MyNamespace;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class PrefixSpec extends ObjectBehavior
      {
          function it_converts_plain_text_to_html_paragraphs()
          {
              $this->toHtml('Hi, there')->shouldReturn('<p>Hi, there</p>');
          }
      }

      """
    And the config file contains:
      """
      suites:
        behat_suite:
          namespace: Behat\Tests\MyNamespace
          psr4_prefix: Behat\Tests
      """
    And the class file "src/MyNamespace/Prefix.php" contains:
      """
      <?php

      namespace Behat\Tests\MyNamespace;

      class Prefix
      {
      }

      """
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then the class in "src/MyNamespace/Prefix.php" should contain:
      """
      <?php

      namespace Behat\Tests\MyNamespace;

      class Prefix
      {

          public function toHtml($argument1)
          {
              // TODO: write logic here
          }
      }

      """
