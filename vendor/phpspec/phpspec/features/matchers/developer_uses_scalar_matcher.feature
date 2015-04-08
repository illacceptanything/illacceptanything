Feature: Developer uses scalar matcher
  As a Developer
  I want an scalar matcher
  In order to match against various scalar values against their expectations

  Scenario: Scalar matching aliases match using the scalar matcher
    Given the spec file "spec/Matchers/ScalarExample1/CarSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ScalarExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class CarSpec extends ObjectBehavior
    {
        function it_returns_the_result()
        {
            $result = $this->getDetails();

            $result['name']->shouldBeString();
            $result['age']->shouldBeInteger();
            $result['price']->shouldBeFloat();
            $result['sale']->shouldBeBool();
            $result['callback']->shouldBeCallable();
        }
    }
    """

    And the class file "src/Matchers/ScalarExample1/Car.php" contains:
    """
    <?php

    namespace Matchers\ScalarExample1;

    class Car
    {
        public function getDetails()
        {
            return array(
                'name' => 'astra',
                'age' => 34,
                'price' => 10.99,
                'sale' => true,
                'callback' => function() {}
            );
        }
    }
    """

    When I run phpspec
    Then the suite should pass
