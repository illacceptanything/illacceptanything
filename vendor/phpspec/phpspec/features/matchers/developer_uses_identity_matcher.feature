Feature: Developer uses identity matcher
  As a Developer
  I want an identity matcher
  In order to match the identity of a value against an expectation

  Scenario: "Return" alias matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample1/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample1;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldReturn(3);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample1/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample1;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Return" alias not matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample2/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample2;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotReturn(4);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample2/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample2;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Return" alias not matching type using identity operator
    Given the spec file "spec/Matchers/IdentityExample3/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample3;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotReturn("3");
          }
      }

      """
    And the class file "src/Matchers/IdentityExample3/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample3;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Be" alias matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample4/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample4;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldBe(3);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample4/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample4;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Be" alias not matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample5/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample5;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotBe(4);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample5/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample5;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Be" alias not matching type using identity operator
    Given the spec file "spec/Matchers/IdentityExample6/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample6;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotBe("3");
          }
      }

      """
    And the class file "src/Matchers/IdentityExample6/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample6;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Equal" alias matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample7/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample7;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldEqual(3);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample7/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample7;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Equal" alias not matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample8/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample8;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotEqual(4);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample8/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample8;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Equal" alias not matching type using identity operator
    Given the spec file "spec/Matchers/IdentityExample9/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample9;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotEqual("3");
          }
      }

      """
    And the class file "src/Matchers/IdentityExample9/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample9;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "BeEqualTo" alias matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample10/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample10;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldEqual(3);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample10/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample10;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Equal" alias not matching using identity operator
    Given the spec file "spec/Matchers/IdentityExample11/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample11;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotEqual(4);
          }
      }

      """
    And the class file "src/Matchers/IdentityExample11/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample11;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: "Equal" alias not matching type using identity operator
    Given the spec file "spec/Matchers/IdentityExample12/CalculatorSpec.php" contains:
      """
      <?php

      namespace spec\Matchers\IdentityExample12;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class CalculatorSpec extends ObjectBehavior
      {
          function it_calculates_the_sum_of_two_addends()
          {
              $this->sum(1, 2)->shouldNotEqual("3");
          }
      }

      """
    And the class file "src/Matchers/IdentityExample12/Calculator.php" contains:
      """
      <?php

      namespace Matchers\IdentityExample12;

      class Calculator
      {
          public function sum($x, $y)
          {
              return $x + $y;
          }
      }

      """
    When I run phpspec
    Then the suite should pass