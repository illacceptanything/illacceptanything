Feature: Developer is told about pending specs
  So that I remember to implement specs
  As a Developer
  I should be told about specs with missing bodies

  Scenario: Empty spec causes pending result
    Given the spec file "spec/Runner/PendingExample1/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\PendingExample1;

      use PhpSpec\ObjectBehavior;

      class MarkdownSpec extends ObjectBehavior
      {
          function it_converts_plain_text_to_html_paragraphs()
          {
          }
      }
      """
    When I run phpspec using the "pretty" format
    Then I should see:
      """
         9  - converts plain text to html paragraphs
              todo: write pending example


      1 specs
      1 examples (1 pending)
      """

  Scenario: Spec with comments causes pending result
    Given the spec file "spec/Runner/PendingExample2/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\PendingExample2;

      use PhpSpec\ObjectBehavior;

      class MarkdownSpec extends ObjectBehavior
      {
          function it_converts_plain_text_to_html_paragraphs()
          {
            /**
            multi-line doc - comment
            */
            /*
            multi-line comment
            */
            // single-line comment
          }
      }
      """
    When I run phpspec using the "pretty" format
    Then I should see:
      """
        9  - converts plain text to html paragraphs
              todo: write pending example


      1 specs
      1 examples (1 pending)
      """

  @issue492
  Scenario: Comments with braces do not confuse the parser
    Given the spec file "spec/Runner/PendingExample3/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\PendingExample3;

      use PhpSpec\ObjectBehavior;

      class MarkdownSpec extends ObjectBehavior
      {
          function it_converts_plain_text_to_html_paragraphs()
          {
              pow(2,2);
              // {
          }
      }
      """
    When I run phpspec using the "pretty" format
    Then I should see:
      """
      1 examples (1 passed)
      """
