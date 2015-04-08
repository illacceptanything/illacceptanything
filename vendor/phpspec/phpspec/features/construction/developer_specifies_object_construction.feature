Feature: Developer specifies object construction
  As a Developer
  I want to describe how objects are constructed
  In order to be able to test objects with non-trivial construction

  Scenario: Class is initialised using a constructor
    Given the spec file "spec/Runner/ConstructorExample1/ClassWithConstructorSpec.php" contains:
      """
      <?php

      namespace spec\Runner\ConstructorExample1;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class ClassWithConstructorSpec extends ObjectBehavior
      {
          function let(\DateTime $date)
          {
              $this->beConstructedWith($date);
          }

          function it_is_initializable()
          {
              $this->shouldHaveType('Runner\ConstructorExample1\ClassWithConstructor');
          }
      }

      """
    And the class file "src/Runner/ConstructorExample1/ClassWithConstructor.php" contains:
      """
      <?php

      namespace Runner\ConstructorExample1;

      class ClassWithConstructor
      {
          private $date;

          public function __construct(\DateTime $date)
          {
              $this->date = $date;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: Class is initialized using a static factory method and a collaborator as argument
    Given the spec file "spec/Runner/ConstructorExample2/ClassWithStaticFactoryMethodSpec.php" contains:
      """
      <?php

      namespace spec\Runner\ConstructorExample2;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class ClassWithStaticFactoryMethodSpec extends ObjectBehavior
      {
          function let(\DateTime $date)
          {
              $this->beConstructedThrough('getInstance', array($date));
          }

          function it_is_initializable()
          {
              $this->shouldHaveType('Runner\ConstructorExample2\ClassWithStaticFactoryMethod');
          }
      }

      """
    And the class file "src/Runner/ConstructorExample2/ClassWithStaticFactoryMethod.php" contains:
      """
      <?php

      namespace Runner\ConstructorExample2;

      class ClassWithStaticFactoryMethod
      {
          private $date;

          public static function getInstance(\DateTime $date)
          {
              return new ClassWithStaticFactoryMethod($date);
          }

          private function __construct(\DateTime $date)
          {
              $this->date = $date;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: Default static constructor parameter is overridden in example
    Given the spec file "spec/Runner/ConstructorExample3/ClassWithConstructorSpec.php" contains:
      """
      <?php

      namespace spec\Runner\ConstructorExample3;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class ClassWithConstructorSpec extends ObjectBehavior
      {
          function let()
          {
              $this->beConstructedWith('foo');
          }

          function it_is_initializable()
          {
              $this->beConstructedWith('bar');
              $this->getType()->shouldReturn('bar');
          }
      }

      """
    And the class file "src/Runner/ConstructorExample3/ClassWithConstructor.php" contains:
      """
      <?php

      namespace Runner\ConstructorExample3;

      class ClassWithConstructor
      {
          private $type;

          public function __construct($type)
          {
              $this->type = $type;
          }

          public function getType()
          {
              return $this->type;
          }
      }

      """
    When I run phpspec
    Then the suite should pass


  Scenario: Static constructor is overridden in example
    Given the spec file "spec/Runner/ConstructorExample4/ClassWithStaticFactoryMethodSpec.php" contains:
      """
      <?php

      namespace spec\Runner\ConstructorExample4;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class ClassWithStaticFactoryMethodSpec extends ObjectBehavior
      {
          function let()
          {
              $this->beConstructedThrough('getInstanceOfType', array('foo'));
          }

          function it_is_initializable()
          {
              $this->beConstructedThrough('getInstanceOfType', array('bar'));
              $this->getType()->shouldReturn('bar');
          }
      }

      """
    And the class file "src/Runner/ConstructorExample4/ClassWithStaticFactoryMethod.php" contains:
      """
      <?php

      namespace Runner\ConstructorExample4;

      class ClassWithStaticFactoryMethod
      {
          private $type;

          private function __construct($type)
          {
              $this->type = $type;
          }

          public static function getInstanceOfType($type)
          {
              return new self($type);
          }

          public function getType()
          {
              return $this->type;
          }
      }

      """
    When I run phpspec
    Then the suite should pass

  Scenario: Static constructor set in example used instead factory method set in let
    Given the spec file "spec/Runner/ConstructorExample7/ClassWithStaticFactoryMethodAndConstructorSpec.php" contains:
    """
    <?php

    namespace spec\Runner\ConstructorExample7;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class ClassWithStaticFactoryMethodAndConstructorSpec extends ObjectBehavior
    {
        function let()
        {
            $this->beConstructedThrough('getInstanceOfType', array('foo'));
        }

        function it_is_initializable()
        {
            $this->beConstructedWith('bar');
            $this->getType()->shouldReturn('bar');
            $this->wasConstructedWith()->shouldReturn('__construct');
        }
    }

    """
    And the class file "src/Runner/ConstructorExample7/ClassWithStaticFactoryMethodAndConstructor.php" contains:
    """
    <?php

    namespace Runner\ConstructorExample7;

    class ClassWithStaticFactoryMethodAndConstructor
    {
        private $type;
        private $wasConstructedWith;

        public function __construct($type)
        {
            $this->type = $type;
            $this->wasConstructedWith = '__construct';
        }

        public static function getInstanceOfType($type)
        {
            $created = new self($type);
            $created->wasConstructedWith = 'getInstanceOfType';
            return $created;
        }

        public function getType()
        {
            return $this->type;
        }

        public function wasConstructedWith()
        {
             return $this->wasConstructedWith;
        }
    }

    """
    When I run phpspec
    Then the suite should pass

  Scenario: Factory method set in example used instead of constructor set in let
    Given the spec file "spec/Runner/ConstructorExample8/ClassWithStaticFactoryMethodAndConstructorSpec.php" contains:
    """
    <?php

    namespace spec\Runner\ConstructorExample8;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class ClassWithStaticFactoryMethodAndConstructorSpec extends ObjectBehavior
    {
        function let()
        {
            $this->beConstructedWith('bar');
            $this->beConstructedThrough('getInstanceOfType', array('foo'));
        }

        function it_is_initializable()
        {
            $this->beConstructedThrough('getInstanceOfType', array('foo'));
            $this->getType()->shouldReturn('foo');
            $this->wasConstructedWith()->shouldReturn('getInstanceOfType');
        }
    }

    """
    And the class file "src/Runner/ConstructorExample8/ClassWithStaticFactoryMethodAndConstructor.php" contains:
    """
    <?php

    namespace Runner\ConstructorExample8;

    class ClassWithStaticFactoryMethodAndConstructor
    {
        private $type;
        private $wasConstructedWith;

        public function __construct($type)
        {
            $this->type = $type;
            $this->wasConstructedWith = '__construct';
        }

        public static function getInstanceOfType($type)
        {
            $created = new self($type);
            $created->wasConstructedWith = 'getInstanceOfType';
            return $created;
        }

        public function getType()
        {
            return $this->type;
        }

        public function wasConstructedWith()
        {
             return $this->wasConstructedWith;
        }
    }

    """
    When I run phpspec
    Then the suite should pass

    Scenario: Developer cannot redefine constructor parameters if object is already instantiated
    Given the spec file "spec/Runner/ConstructorExample9/ClassWithConstructorSpec.php" contains:
    """
    <?php

    namespace spec\Runner\ConstructorExample9;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class ClassConstructorSpec extends ObjectBehavior
    {
        function it_behaves_differently_depending_on_type()
        {
            $this->beConstructedWith('foo');
            $this->getType()->shouldReturn('foo');

            $this->beConstructedWith('bar');
            $this->getType()->shouldReturn('bar');
        }
    }

    """
    And the class file "src/Runner/ConstructorExample9/ClassConstructor.php" contains:
    """
    <?php

    namespace Runner\ConstructorExample9;

    class ClassConstructor
    {
        public function __construct($type)
        {
            $this->type = $type;
        }

        public function getType()
        {
            return $this->type;
        }
    }

    """
    When I run phpspec
    Then I should see "you can not change object construction method when it is already instantiated"

  Scenario: Developer cannot redefine factory method if object is already instantiated
    Given the spec file "spec/Runner/ConstructorExample10/ClassWithFactoryMethodSpec.php" contains:
    """
    <?php

    namespace spec\Runner\ConstructorExample10;

    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class ClassWithFactoryMethodSpec extends ObjectBehavior
    {
        function it_behaves_differently_depending_on_type()
        {
            $this->beConstructedThrough('createFoo');
            $this->getType()->shouldReturn('foo');

            $this->beConstructedWith('createBar');
            $this->getType()->shouldReturn('bar');
        }
    }

    """
    And the class file "src/Runner/ConstructorExample10/ClassWithFactoryMethod.php" contains:
    """
    <?php

    namespace Runner\ConstructorExample10;

    class ClassWithFactoryMethod
    {
        private function __construct($type)
        {
            $this->type = $type;
        }

        public function getType()
        {
            return $this->type;
        }

        public static function createFoo()
        {
            return new self('foo');
        }

        public static function createBar()
        {
            return new self('bar');
        }
    }

    """
    When I run phpspec
    Then I should see "you can not change object construction method when it is already instantiated"
