Feature: Use the JUnit formatter
  In order to provide my CI tool with parsable phpspec results
  As a developer
  I need to be able to use a JUnit formatter

  Scenario: Successfully export phpspec results in JUnit format
    Given the spec file "spec/Formatter/SpecExample/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Formatter\SpecExample;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class MarkdownSpec extends ObjectBehavior
      {
          // passed
          function it_converts_plain_text_to_html_paragraphs()
          {
              $this->toHtml('Hi, there')->shouldReturn('<p>Hi, there</p>');
          }

          // pending
          function it_converts_html_paragraph_to_plain_text()
          {
          }

          // failed
          function it_formats_asterik_surrounding_text_in_italic()
          {
              $this->toHtml('*How are you?*')->shouldReturn('<i>How are you?</i>');
          }

          // broken
          function it_formats_empty_text()
          {
              $this->toHtml('')->shouldReturn('<p></p>');
          }

          // skipped
          function it_does_some_incompatible_things()
          {
              throw new \PhpSpec\Exception\Example\SkippingException();
          }
      }

      """
    And the class file "src/Formatter/SpecExample/Markdown.php" contains:
      """
      <?php

      namespace Formatter\SpecExample;

      class Markdown
      {
          public function toHtml($text)
          {
              if (empty($text)) {
                  throw new \InvalidArgumentException('Text cannot be empty: &$£€<>"');
              }
              return sprintf('<p>%s</p>', $text);
          }
      }

      """
    When I run phpspec using the "junit" format
    Then I should see valid junit output
