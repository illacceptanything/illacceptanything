Feature: Developer uses array-key matcher
  As a Developer
  I want an array-key matcher
  In order to confirm an array contains an expected key

  Scenario: "HaveKey" alias matches using the array-key matcher
    Given the spec file "spec/Matchers/ArrayKeyExample1/MovieSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ArrayKeyExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class MovieSpec extends ObjectBehavior
    {
        function it_should_have_a_release_date_for_france()
        {
            $this->getReleaseDates()->shouldHaveKey('France');
        }
    }
    """

    And the class file "src/Matchers/ArrayKeyExample1/Movie.php" contains:
    """
    <?php

    namespace Matchers\ArrayKeyExample1;

    class Movie
    {
        public function getReleaseDates()
        {
            return array(
                'Australia' => '12 April 2013',
                'France'    => '24 April 2013',
            );
        }
    }
    """

    When I run phpspec
    Then the suite should pass
