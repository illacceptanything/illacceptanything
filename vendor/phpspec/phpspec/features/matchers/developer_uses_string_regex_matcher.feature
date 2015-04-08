Feature: Developer uses string-regex matcher
  As a Developer
  I want an string-regex matcher
  In order to confirm a string matches against an expected regular expression

  Scenario: "Matches" alias matches using the string-regex matcher
    Given the spec file "spec/Matchers/StringRegexExample1/MovieSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\StringRegexExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class MovieSpec extends ObjectBehavior
    {
        function it_should_have_a_title_that_contains_wizard()
        {
            $this->getTitle()->shouldMatch('/wizard/i');
        }
    }
    """

    And the class file "src/Matchers/StringRegexExample1/Movie.php" contains:
    """
    <?php

    namespace Matchers\StringRegexExample1;

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
