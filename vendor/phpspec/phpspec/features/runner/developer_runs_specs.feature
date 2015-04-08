Feature: Developer runs the specs
  As a Developer
  I want to run the specs
  In order to get feedback on a state of my application

  Scenario: Running a spec with a class that doesn't exist
    Given I have started describing the "Runner/SpecExample1/Markdown" class
    When I run phpspec
    Then I should see "class Runner\SpecExample1\Markdown does not exist"

  Scenario: Reporting success when running a spec with correctly implemented class
    Given the spec file "spec/Runner/SpecExample2/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\SpecExample2;

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
    And the class file "src/Runner/SpecExample2/Markdown.php" contains:
      """
      <?php

      namespace Runner\SpecExample2;

      class Markdown
      {
          public function toHtml($text)
          {
              return sprintf('<p>%s</p>', $text);
          }
      }

      """
    When I run phpspec
    Then the suite should pass

    @issue214
  Scenario: Letgo is executed after successful spec
    Given the spec file "spec/Runner/SpecExample3/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\SpecExample3;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class MarkdownSpec extends ObjectBehavior
      {
          function letgo()
          {
              throw new \Exception('Letgo is called');
          }

          function it_converts_plain_text_to_html_paragraphs()
          {
              $this->toHtml('Hi, there')->shouldReturn('<p>Hi, there</p>');
          }
      }

      """
    And the class file "src/Runner/SpecExample3/Markdown.php" contains:
      """
      <?php

      namespace Runner\SpecExample3;

      class Markdown
      {
          public function toHtml($text)
          {
              return sprintf('<p>%s</p>', $text);
          }
      }

      """
    When I run phpspec
    Then I should see "Letgo is called"

    @issue214
  Scenario: Letgo is executed after exception is thrown
    Given the spec file "spec/Runner/SpecExample4/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\SpecExample4;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class MarkdownSpec extends ObjectBehavior
      {
          function letgo()
          {
              throw new \Exception('Letgo is called');
          }

          function it_converts_plain_text_to_html_paragraphs()
          {
              $this->toHtml('Hi, there')->shouldReturn('<p>Hi, there</p>');
          }
      }

      """
    And the class file "src/Runner/SpecExample4/Markdown.php" contains:
      """
      <?php

      namespace Runner\SpecExample4;

      class Markdown
      {
          public function toHtml($text)
          {
              throw new \Exception('Some exception');
          }
      }

      """
    When I run phpspec
    Then I should see "Letgo is called"
