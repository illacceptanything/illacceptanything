Feature: Developer uses throw matcher
  As a Developer
  I want a throw matcher
  In order to validate objects exceptions against my expectations

  Scenario: "Throw" alias matches using the throw matcher with explicit method name
    Given the spec file "spec/Matchers/ThrowExample1/EmployeeSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ThrowExample1;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class EmployeeSpec extends ObjectBehavior
    {
        function it_throws_an_exception_when_arguments_are_invalid()
        {
            $this->shouldThrow('\InvalidArgumentException')->during('setAge', array(0));
        }
    }
    """

    And the class file "src/Matchers/ThrowExample1/Employee.php" contains:
    """
    <?php

    namespace Matchers\ThrowExample1;

    class Employee
    {
        public function setAge($age)
        {
            if (0 === $age) {
                throw new \InvalidArgumentException();
            }
        }
    }
    """

    When I run phpspec
    Then the suite should pass

  Scenario: "Throw" alias matches using the throw matcher with implicit method name
    Given the spec file "spec/Matchers/ThrowExample2/EmployeeSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ThrowExample2;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class EmployeeSpec extends ObjectBehavior
    {
        function it_throws_an_exception_when_arguments_are_invalid()
        {
            $this->shouldThrow('\InvalidArgumentException')->duringSetAge(0);
        }
    }
    """

    And the class file "src/Matchers/ThrowExample2/Employee.php" contains:
    """
    <?php

    namespace Matchers\ThrowExample2;

    class Employee
    {
        public function setAge($age)
        {
            if (0 === $age) {
                throw new \InvalidArgumentException();
            }
        }
    }
    """

    When I run phpspec
    Then the suite should pass


  Scenario: "Throw" alias matches using the throw matcher with specific exception message
    Given the spec file "spec/Matchers/ThrowExample3/EmployeeSpec.php" contains:
    """
    <?php

    namespace spec\Matchers\ThrowExample3;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class EmployeeSpec extends ObjectBehavior
    {
        function it_throws_an_exception_when_arguments_are_invalid()
        {
            $this->shouldThrow(new \InvalidArgumentException('Invalid age'))->duringSetAge(0);
        }
    }
    """

    And the class file "src/Matchers/ThrowExample3/Employee.php" contains:
    """
    <?php

    namespace Matchers\ThrowExample3;

    class Employee
    {
        public function setAge($age)
        {
            if (0 === $age) {
                throw new \InvalidArgumentException('Invalid age');
            }
        }
    }
    """

    When I run phpspec
    Then the suite should pass

  @issue134
  Scenario: Throwing an exception during object construction
    Given the spec file "spec/Runner/ThrowExample4/MarkdownSpec.php" contains:
    """
    <?php

    namespace spec\Runner\ThrowExample4;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class MarkdownSpec extends ObjectBehavior
    {
        function it_throws_an_exception_using_during_syntax()
        {
            $this->shouldThrow('Exception')->during('__construct', array(1,2));
        }

        function it_throws_an_exception_using_magic_syntax()
        {
            $this->shouldThrow('Exception')->during__construct(1,2);
        }
    }

    """
    And the class file "src/Runner/ThrowExample4/Markdown.php" contains:
    """
    <?php

    namespace Runner\ThrowExample4;

    class Markdown
    {
        public function __construct($num1, $num2)
        {
            throw new \Exception();
        }
    }

    """
    When I run phpspec
    Then the suite should pass
