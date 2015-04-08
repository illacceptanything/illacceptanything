<?php

use Behat\Behat\Tester\Exception\PendingException;
use Behat\Behat\Context\Context;
use Behat\Gherkin\Node\PyStringNode;
use Behat\Gherkin\Node\TableNode;
use Matcher\FileExistsMatcher;
use Matcher\FileHasContentsMatcher;
use PhpSpec\Matcher\MatchersProviderInterface;
use Symfony\Component\Filesystem\Filesystem;

/**
 * Defines application features from the specific context.
 */
class FilesystemContext implements Context, MatchersProviderInterface
{
    /**
     * @var string
     */
    private $workingDirectory;

    /**
     * @var Filesystem
     */
    private $filesystem;

    public function __construct()
    {
        $this->filesystem = new Filesystem();
    }

    /**
     * @beforeScenario
     */
    public function prepWorkingDirectory()
    {
        $this->workingDirectory = tempnam(sys_get_temp_dir(), 'phpspec-behat');
        $this->filesystem->remove($this->workingDirectory);
        $this->filesystem->mkdir($this->workingDirectory);
        chdir($this->workingDirectory);
    }

    /**
     * @afterScenario
     */
    public function removeWorkingDirectory()
    {
        $this->filesystem->remove($this->workingDirectory);
    }

    /**
     * @Given the bootstrap file :file contains:
     */
    public function theFileContains($file, PyStringNode $contents)
    {
        $this->filesystem->dumpFile($file, (string)$contents);
    }

    /**
     * @Given the class file :file contains:
     * @Given the spec file :file contains:
     */
    public function theClassOrSpecFileContains($file, PyStringNode $contents)
    {
        $this->theFileContains($file, $contents);
        require_once($file);
    }

    /**
     * @Given the config file contains:
     */
    public function theConfigFileContains(PyStringNode $contents)
    {
        $this->theFileContains('phpspec.yml', $contents);
    }

    /**
     * @Given there is no file :file
     */
    public function thereIsNoFile($file)
    {
        expect($file)->toNotExist();
        expect(file_exists($file))->toBe(false);
    }

    /**
     * @Then the class in :file should contain:
     * @Then a new class/spec should be generated in the :file:
     */
    public function theFileShouldContain($file, PyStringNode $contents)
    {
        expect($file)->toExist();
        expect($file)->toHaveContents($contents);
    }

    /**
     * @return array
     */
    public function getMatchers()
    {
        return array(
            new FileExistsMatcher(),
            new FileHasContentsMatcher()
        );
    }
}
