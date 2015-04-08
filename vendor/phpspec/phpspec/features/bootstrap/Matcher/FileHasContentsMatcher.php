<?php

namespace Matcher;

use PhpSpec\Exception\Example\FailureException;
use PhpSpec\Matcher\MatcherInterface;

class FileHasContentsMatcher implements MatcherInterface
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
        return ('haveContents' == $name && is_string($subject));
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
        $path = $subject;
        $expectedContents = $arguments[0];
        if ($expectedContents != file_get_contents($path)) {
            throw new FailureException(sprintf(
                "File at '%s' did not contain expected contents.\nExpected: '%s'\nActual: '%s'",
                $path,
                $expectedContents,
                file_get_contents($path)
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
        throw new FailureException('Negative file contents matcher not implemented');
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