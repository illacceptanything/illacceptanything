Feature: Developer uses string-end matcher
  As a Developer
  I want an string-end matcher
  In order to confirm a string ends with an expected substring

  Scenario: "EndsWith" alias matches using the string-end matcher
    Given the spec file "spec/Matchers/StringEndExample1/MovieSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\StringEndExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class MovieSpec extends ObjectBehavior
    {
        function it_should_have_a_title_that_ends_with_of_oz()
        {
            $this->getTitle()->shouldEndWith('of Oz');
        }
    }
    """

    And the class file "src/Matchers/StringEndExample1/Movie.php" contains:
    """
    <?php

    namespace Matchers\StringEndExample1;

    class Movie
    {
        public function getTitle()
        {
            return 'The Wizard of Oz';
        }
    }
    """

    When I run phpspec
    Then the suite should pass
