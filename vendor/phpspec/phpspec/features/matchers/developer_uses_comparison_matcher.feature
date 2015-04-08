Feature: Developer uses comparison matcher
  As a Developer
  I want a comparison matcher
  In order to loosely compare a value against an expectation

  Scenario: "BeLike" alias matches using comparison operator
    Given the spec file "spec/Matchers/ComparisonExample1/StringCalculatorSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ComparisonExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class StringCalculatorSpec extends ObjectBehavior
    {
        function it_returns_the_value_of_a_string()
        {
            $this->calc('5')->shouldBeLike('5');
        }
    }
    """

    And the class file "src/Matchers/ComparisonExample1/StringCalculator.php" contains:
    """
    <?php

    namespace Matchers\ComparisonExample1;

    class StringCalculator
    {
        public function calc($string)
        {
            return (int) $string;
        }
    }
    """

    When I run phpspec
    Then the suite should pass
