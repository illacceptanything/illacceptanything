Feature: Developer is shown diffs
  In order to debug failing tests
  As a developer
  I should be shown a detailed diff when expected values do not match

  Scenario: String diffing
    Given the spec file "spec/Diffs/DiffExample1/ClassWithStringsSpec.php" contains:
      """
      <?php

      namespace spec\Diffs\DiffExample1;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class ClassWithStringsSpec extends ObjectBehavior
      {
          function it_is_equal()
          {
              $this->getString()->shouldReturn('foo');
          }
      }

      """
    And the class file "src/Diffs/DiffExample1/ClassWithStrings.php" contains:
      """
      <?php

      namespace Diffs\DiffExample1;

      class ClassWithStrings
      {
          public function getString()
          {
              return 'bar';
          }
      }

      """
    When I run phpspec with the "verbose" option
    Then I should see:
      """
       @@ -1,1 +1,1 @@
            -foo
            +bar
      """

  Scenario: Array diffing
    Given the spec file "spec/Diffs/DiffExample2/ClassWithArraysSpec.php" contains:
      """
      <?php

      namespace spec\Diffs\DiffExample2;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class ClassWithArraysSpec extends ObjectBehavior
      {
          function it_is_equal()
          {
              $this->getArray()->shouldReturn(array(
                'int' => 1,
                'string' => 'foo'
              ));
          }
      }

      """
    And the class file "src/Diffs/DiffExample2/ClassWithArrays.php" contains:
      """
      <?php

      namespace Diffs\DiffExample2;

      class ClassWithArrays
      {
          public function getArray()
          {
              return array(
                'int' => 3,
                'string' => 'bar'
              );
          }
      }

      """
    When I run phpspec with the "verbose" option
    Then I should see:
      """
            @@ -1,4 +1,4 @@
               [
            -    int => 1,
            -    string => ""foo"...",
            +    int => 3,
            +    string => ""bar"...",
               ]
      """

  Scenario: Object diffing
    Given the spec file "spec/Diffs/DiffExample3/ClassWithObjectsSpec.php" contains:
      """
      <?php

      namespace spec\Diffs\DiffExample3;

      use PhpSpec\ObjectBehavior;
      use Prophecy\Argument;

      class ClassWithObjectsSpec extends ObjectBehavior
      {
          function it_is_equal()
          {
              $obj = new \StdClass;
              $obj->i = 1;
              $obj->s = 'foo';

              $this->getObject()->shouldReturn($obj);
          }
      }

      """
    And the class file "src/Diffs/DiffExample3/ClassWithObjects.php" contains:
      """
      <?php

      namespace Diffs\DiffExample3;

      class ClassWithObjects
      {
          public function getObject()
          {
              $obj = new \StdClass;
              $obj->i = 2;
              $obj->s = 'bar';

              return $obj;
          }
      }

      """
    When I run phpspec with the "verbose" option
    Then I should see:
      """
            -    'i' => 1
            -    's' => 'foo'
      """
    And I should see:
      """
            +    'i' => 2
            +    's' => 'bar'
      """
