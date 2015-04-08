Feature: Developer uses object-state matcher
  As a Developer
  I want an object-state matcher
  In order to validate objects against an expectation

  Scenario: "Have" alias matches using the object-state matcher
    Given the spec file "spec/Matchers/ObjectStateExample1/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ObjectStateExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_returns_true_if_it_has_an_wheels()
        {
            $this->shouldHaveWheels();
        }
    }
    """

    And the class file "src/Matchers/ObjectStateExample1/Car.php" contains:
    """
    <?php

    namespace Matchers\ObjectStateExample1;

    class Car
    {
        public function hasWheels()
        {
            return true;
        }
    }
    """

    When I run phpspec
    Then the suite should pass

  Scenario: "Be" alias matches using the object-state matcher
    Given the spec file "spec/Matchers/ObjectStateExample2/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ObjectStateExample2;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_returns_true_if_it_is_available()
        {
            $this->shouldBeAvailable();
        }
    }
    """

    And the class file "src/Matchers/ObjectStateExample2/Car.php" contains:
    """
    <?php

    namespace Matchers\ObjectStateExample2;

    class Car
    {
        public function isAvailable()
        {
            return true;
        }
    }
    """

    When I run phpspec
    Then the suite should pass