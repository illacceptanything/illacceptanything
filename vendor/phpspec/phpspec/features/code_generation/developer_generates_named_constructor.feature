Feature: Developer generates a named constructor
  As a Developer
  I want to automate creating named constructor
  In order to avoid repetitive tasks and interruptions in development flow

  Scenario: Generating a named constructor in an empty class
  Given the spec file "spec/CodeGeneration/NamedConstructor/UserSpec.php" contains:
    """
    <?php

    namespace spec\CodeGeneration\NamedConstructor;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class UserSpec extends ObjectBehavior
    {
        function it_registers_a_user()
        {
            $this->beConstructedThrough('register', array('firstname', 'lastname'));
            $this->getFirstname()->shouldBe('firstname');
        }
    }

    """
  And the class file "src/CodeGeneration/NamedConstructor/User.php" contains:
    """
    <?php

    namespace CodeGeneration\NamedConstructor;

    class User
    {
    }

    """
  When I run phpspec and answer "y" when asked if I want to generate the code
  Then the class in "src/CodeGeneration/NamedConstructor/User.php" should contain:
    """
    <?php

    namespace CodeGeneration\NamedConstructor;

    class User
    {

        public static function register($argument1, $argument2)
        {
            $user = new User();

            // TODO: write logic here

            return $user;
        }
    }

    """

  Scenario: Generating a named constructor with more arguments than an existing constructor accepts
  Given the spec file "spec/CodeGeneration/NamedConstructor/TooManyArguments/UserSpec.php" contains:
    """
    <?php

    namespace spec\CodeGeneration\NamedConstructor\TooManyArguments;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class UserSpec extends ObjectBehavior
    {
        function it_registers_a_user()
        {
            $this->beConstructedThrough('register', array('firstname', 'lastname'));
            $this->getFirstname()->shouldBe('firstname');
        }
    }

    """
  And the class file "src/CodeGeneration/NamedConstructor/TooManyArguments/User.php" contains:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\TooManyArguments;

    class User
    {
        public function __construct()
        {
        }
    }

    """
  When I run phpspec and answer "y" when asked if I want to generate the code
  Then the class in "src/CodeGeneration/NamedConstructor/TooManyArguments/User.php" should contain:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\TooManyArguments;

    class User
    {
        public function __construct()
        {
        }

        public static function register($argument1, $argument2)
        {
            throw new \BadMethodCallException("Mismatch between the number of arguments of the factory method and constructor");
        }
    }

    """

  Scenario: Generating a named constructor with less arguments than an existing constructor accepts
  Given the spec file "spec/CodeGeneration/NamedConstructor/TooFewArguments/UserSpec.php" contains:
    """
    <?php

    namespace spec\CodeGeneration\NamedConstructor\TooFewArguments;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class UserSpec extends ObjectBehavior
    {
        function it_registers_a_user()
        {
            $this->beConstructedThrough('register', array('firstname', 'lastname'));
            $this->getFirstname()->shouldBe('firstname');
        }
    }

    """
  And the class file "src/CodeGeneration/NamedConstructor/TooFewArguments/User.php" contains:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\TooFewArguments;

    class User
    {
        public function __construct($argument1, $argument2, $argument3)
        {
        }
    }

    """
  When I run phpspec and answer "y" when asked if I want to generate the code
  Then the class in "src/CodeGeneration/NamedConstructor/TooFewArguments/User.php" should contain:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\TooFewArguments;

    class User
    {
        public function __construct($argument1, $argument2, $argument3)
        {
        }

        public static function register($argument1, $argument2)
        {
            throw new \BadMethodCallException("Mismatch between the number of arguments of the factory method and constructor");
        }
    }

    """

  Scenario: Generating a named constructor with a matching number of constructor arguments
    Given the spec file "spec/CodeGeneration/NamedConstructor/EqualArguments/UserSpec.php" contains:
    """
    <?php

    namespace spec\CodeGeneration\NamedConstructor\EqualArguments;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class UserSpec extends ObjectBehavior
    {
        function it_registers_a_user()
        {
            $this->beConstructedThrough('register', array('firstname', 'lastname'));
            $this->getFirstname()->shouldBe('firstname');
        }
    }

    """
    And the class file "src/CodeGeneration/NamedConstructor/EqualArguments/User.php" contains:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\EqualArguments;

    class User
    {
        public function __construct($argument1, $argument2)
        {
        }
    }

    """
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then the class in "src/CodeGeneration/NamedConstructor/EqualArguments/User.php" should contain:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\EqualArguments;

    class User
    {
        public function __construct($argument1, $argument2)
        {
        }

        public static function register($argument1, $argument2)
        {
            $user = new User($argument1, $argument2);

            // TODO: write logic here

            return $user;
        }
    }

    """

  Scenario: Generating a named constructor with the correct number of required constructor arguments
  Given the spec file "spec/CodeGeneration/NamedConstructor/OptionalArguments/UserSpec.php" contains:
    """
    <?php

    namespace spec\CodeGeneration\NamedConstructor\OptionalArguments;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class UserSpec extends ObjectBehavior
    {
        function it_registers_a_user()
        {
            $this->beConstructedThrough('register', array('firstname', 'lastname'));
            $this->getFirstname()->shouldBe('firstname');
        }
    }

    """
  And the class file "src/CodeGeneration/NamedConstructor/OptionalArguments/User.php" contains:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\OptionalArguments;

    class User
    {
        public function __construct($argument1, $argument2, $argument3 = 'optional')
        {
        }
    }

    """
  When I run phpspec and answer "y" when asked if I want to generate the code
  Then the class in "src/CodeGeneration/NamedConstructor/OptionalArguments/User.php" should contain:
    """
    <?php

    namespace CodeGeneration\NamedConstructor\OptionalArguments;

    class User
    {
        public function __construct($argument1, $argument2, $argument3 = 'optional')
        {
        }

        public static function register($argument1, $argument2)
        {
            $user = new User($argument1, $argument2);

            // TODO: write logic here

            return $user;
        }
    }

    """
