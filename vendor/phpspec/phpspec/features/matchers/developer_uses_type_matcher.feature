Feature: Developer uses type matcher
  As a Developer
  I want a type matcher
  In order to confirm that my object is of a given type

  Scenario: "HaveType" alias matches using the type matcher
    Given the spec file "spec/Matchers/TypeExample1/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\TypeExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_should_be_a_car()
        {
            $this->shouldHaveType('Matchers\TypeExample1\Car');
        }
    }
    """

    And the class file "src/Matchers/TypeExample1/Car.php" contains:
    """
    <?php

    namespace Matchers\TypeExample1;

    class Car
    {
    }
    """

    When I run phpspec
    Then the suite should pass


  Scenario: "ReturnAnInstanceOf" alias matches using the type matcher
    Given the spec file "spec/Matchers/TypeExample2/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\TypeExample2;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_should_be_a_car()
        {
            $this->get()->shouldReturnAnInstanceOf('Matchers\TypeExample2\Car');
        }
    }
    """

    And the class file "src/Matchers/TypeExample2/Car.php" contains:
    """
    <?php

    namespace Matchers\TypeExample2;

    class Car
    {
        public function get()
        {
            return $this;
        }
    }
    """

    When I run phpspec
    Then the suite should pass

  Scenario: "BeAnInstanceOf" alias matches using the type matcher
    Given the spec file "spec/Matchers/TypeExample3/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\TypeExample3;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_should_be_a_car()
        {
            $this->get()->shouldBeAnInstanceOf('Matchers\TypeExample3\Car');
        }
    }
    """

    And the class file "src/Matchers/TypeExample3/Car.php" contains:
    """
    <?php

    namespace Matchers\TypeExample3;

    class Car
    {
        public function get()
        {
            return $this;
        }
    }
    """

    When I run phpspec
    Then the suite should pass

  Scenario: "Implement" alias matches using the type matcher
    Given the spec file "spec/Matchers/TypeExample4/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\TypeExample4;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_should_be_a_car()
        {
            $this->shouldImplement('Matchers\TypeExample4\Car');
        }
    }
    """

    And the class file "src/Matchers/TypeExample4/Car.php" contains:
    """
    <?php

    namespace Matchers\TypeExample4;

    class Car
    {
    }
    """

    When I run phpspec
    Then the suite should pass
