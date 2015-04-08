Feature: Developer uses string-start matcher
  As a Developer
  I want an string-start matcher
  In order to confirm a string starts with an expected substring

  Scenario: "StartWith" alias matches using the string-start matcher
    Given the spec file "spec/Matchers/StringStartExample1/MovieSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\StringStartExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class MovieSpec extends ObjectBehavior
    {
        function it_should_have_a_title_that_starts_with_the_wizard()
        {
            $this->getTitle()->shouldStartWith('The Wizard');
        }
    }
    """

    And the class file "src/Matchers/StringStartExample1/Movie.php" contains:
    """
    <?php

    namespace Matchers\StringStartExample1;

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
