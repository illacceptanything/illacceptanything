Feature: Developer skips examples
  As a Developer
  I want to skip some examples I know won't pass
  In order to sanitize my result output

  Scenario: Skip a spec with and run it using the dot formatter
    Given the spec file "spec/Runner/SpecExample/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\SpecExample;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;
      use PhpSpec\Exception\Example\SkippingException;

      class MarkdownSpec extends ObjectBehavior
      {
          function it_converts_plain_text_table_to_html_table()
          {
              throw new SkippingException('subject to a php bug');
          }
      }

      """
    And the class file "src/Runner/SpecExample/Markdown.php" contains:
      """
      <?php

      namespace Runner\SpecExample;

      class Markdown
      {
          public function toHtml($text)
          {
          }
      }

      """
    When I run phpspec using the "dot" format
    Then 1 example should have been skipped
    But the suite should pass
