Feature: Developer generates a class
  As a Developer
  I want to automate creating classes
  In order to avoid repetitive tasks and interruptions in development flow

  Scenario: Generating a class
    Given I have started describing the "CodeGeneration/ClassExample1/Markdown" class
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then a new class should be generated in the "src/CodeGeneration/ClassExample1/Markdown.php":
      """
      <?php

      namespace CodeGeneration\ClassExample1;

      class Markdown
      {
      }

      """

  @issue269
  Scenario: Generating a class with psr4 prefix
    Given the config file contains:
    """
    suites:
      behat_suite:
        namespace: Behat\Tests\MyNamespace
        psr4_prefix: Behat\Tests
    """
    And I have started describing the "Behat/Tests/MyNamespace/Markdown" class
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then a new class should be generated in the "src/MyNamespace/Markdown.php":
    """
    <?php

    namespace Behat\Tests\MyNamespace;

    class Markdown
    {
    }

    """

  @issue127
  Scenario: Generating a class with PSR0 must convert classname underscores to directory separator
    Given I have started describing the "CodeGeneration/ClassExample1/Text_Markdown" class
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then a new class should be generated in the "src/CodeGeneration/ClassExample1/Text/Markdown.php":
      """
      <?php

      namespace CodeGeneration\ClassExample1;

      class Text_Markdown
      {
      }

      """

  @issue127
  Scenario: Generating a class with PSR0 must not convert namespace underscores to directory separator
    Given I have started describing the "CodeGeneration/Class_Example2/Text_Markdown" class
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then a new class should be generated in the "src/CodeGeneration/Class_Example2/Text/Markdown.php":
      """
      <?php

      namespace CodeGeneration\Class_Example2;

      class Text_Markdown
      {
      }

      """

  Scenario: Generating a class when expectations on collaborator are defined
    Given the spec file "spec/CodeGeneration/MethodExample2/ForgotPasswordSpec.php" contains:
    """
    <?php

    namespace spec\CodeGeneration\MethodExample2;

    use CodeGeneration\MethodExample2\UserRepository;
    use CodeGeneration\MethodExample2\User;
    use PhpSpec\ObjectBehavior;
    use Prophecy\Argument;

    class ForgotPasswordSpec extends ObjectBehavior
    {
        function it_changes_password_for_user(UserRepository $repository, User $user)
        {
            $repository->findOneByEmail('leszek.prabucki@gmail.com')->willReturn($user);
            $user->changePassword('123')->shouldBeCalled();

            $this->changePassword('leszek.prabucki@gmail.com', '123');
        }
    }
    """
    And the class file "src/CodeGeneration/MethodExample2/User.php" contains:
    """
    <?php

    namespace CodeGeneration\MethodExample2;

    interface User
    {
        public function changePassword($newPassword);
    }
    """
    And the class file "src/CodeGeneration/MethodExample2/UserRepository.php" contains:
    """
    <?php

    namespace CodeGeneration\MethodExample2;

    interface UserRepository
    {
        public function findOneByEmail($email);
    }
    """
    When I run phpspec and answer "y" when asked if I want to generate the code
    Then the class in "src/CodeGeneration/MethodExample2/ForgotPassword.php" should contain:
    """
    <?php

    namespace CodeGeneration\MethodExample2;

    class ForgotPassword
    {
    }

    """
