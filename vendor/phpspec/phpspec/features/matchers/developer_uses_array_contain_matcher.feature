Feature: Developer uses array-contain matcher
  As a Developer
  I want an array-contain matcher
  In order to confirm an array contains an expected value

  Scenario: "Contain" alias matches using the array-contain matcher
    Given the spec file "spec/Matchers/ArrayContainExample1/MovieSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ArrayContainExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class MovieSpec extends ObjectBehavior
    {
        function it_should_contain_jane_smith_in_the_cast()
        {
            $this->getCast()->shouldContain('Jane Smith');
        }
    }
    """

    And the class file "src/Matchers/ArrayContainExample1/Movie.php" contains:
    """
    <?php

    namespace Matchers\ArrayContainExample1;

    class Movie
    {
        public function getCast()
        {
            return array('John Smith', 'Jane Smith');
        }
    }
    """

    When I run phpspec
    Then the suite should pass
