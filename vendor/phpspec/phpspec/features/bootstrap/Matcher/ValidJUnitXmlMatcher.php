<?php

namespace Matcher;

use PhpSpec\Exception\Example\FailureException;
use PhpSpec\Matcher\MatcherInterface;
use Symfony\Component\Console\Tester\ApplicationTester;

const JUNIT_XSD_PATH = '/src/PhpSpec/Resources/schema/junit.xsd';

class ValidJUnitXmlMatcher implements MatcherInterface
{
    /**
     * Checks if matcher supports provided subject and matcher name.
     *
     * @param string $name
     * @param mixed $subject
     * @param array $arguments
     *
     * @return Boolean
     */
    public function supports($name, $subject, array $arguments)
    {
        return ($name == 'haveOutputValidJunitXml' && $subject instanceof ApplicationTester);
    }

    /**
     * Evaluates positive match.
     *
     * @param string $name
     * @param mixed $subject
     * @param array $arguments
     */
    public function positiveMatch($name, $subject, array $arguments)
    {
        $dom = new \DOMDocument();
        $dom->loadXML($subject->getDisplay());
        if (!$dom->schemaValidate(__DIR__ . '/../../..' . JUNIT_XSD_PATH)) {
            throw new FailureException(sprintf(
               "Output was not valid JUnit XML"
            ));
        }
    }

    /**
     * Evaluates negative match.
     *
     * @param string $name
     * @param mixed $subject
     * @param array $arguments
     */
    public function negativeMatch($name, $subject, array $arguments)
    {
        throw new FailureException('Negative JUnit matcher not implemented');
    }

    /**
     * Returns matcher priority.
     *
     * @return integer
     */
    public function getPriority()
    {
        return 51;
    }
}