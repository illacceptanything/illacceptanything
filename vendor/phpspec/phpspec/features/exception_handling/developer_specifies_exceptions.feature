Feature: Developer specifies exception behaviour
  As a Developer
  I want to be able to specify the exceptions by SUS will throw
  In order to drive the design of my exception handling

  Scenario: Throwing an exception during construction when beConstructedWith specifies valid parameters
    Given the spec file "spec/Runner/ExceptionExample3/MarkdownSpec.php" contains:
      """
      <?php

      namespace spec\Runner\ExceptionExample3;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class MarkdownSpec extends ObjectBehavior
      {
          function let()
          {
              $this->beConstructedWith('nothrow');
          }

          function it_throws_an_exception_using_magic_syntax()
          {
              $this->shouldThrow('Exception')->during__construct('throw');
          }
      }

      """
    And the class file "src/Runner/ExceptionExample3/Markdown.php" contains:
      """
      <?php

      namespace Runner\ExceptionExample3;

      class Markdown
      {
          public function __construct($param)
          {
              if ($param == 'throw') {
                  throw new \Exception();
              }
          }
      }

      """
    When I run phpspec
    Then the suite should pass