Feature: Developer uses array-count matcher
  As a Developer
  I want an array-count matcher
  In order to compare an array count against an expectation

  Scenario: "HaveCount" alias matches using the array-count matcher
    Given the spec file "spec/Matchers/ArrayCountExample1/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ArrayCountExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_returns_the_number_of_wheels()
        {
            $this->getWheels()->shouldHaveCount(4);
        }
    }
    """

    And the class file "src/Matchers/ArrayCountExample1/Car.php" contains:
    """
    <?php

    namespace Matchers\ArrayCountExample1;

    class Car
    {
        public function getWheels()
        {
            return array('wheel', 'wheel', 'wheel', 'wheel');
        }
    }
    """

    When I run phpspec
    Then the suite should pass
