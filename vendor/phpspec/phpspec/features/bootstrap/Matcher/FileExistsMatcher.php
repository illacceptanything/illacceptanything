<?php

namespace Matcher;

use PhpSpec\Exception\Example\FailureException;
use PhpSpec\Matcher\MatcherInterface;

class FileExistsMatcher implements MatcherInterface
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
        return ('exist' == $name && is_string($subject));
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
        if (!file_exists($subject)) {
            throw new FailureException(sprintf(
                "File did not exist at path '%s'",
                $subject
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
        if (file_exists($subject)) {
            throw new FailureException(sprintf(
                "File unexpectedly exists at path '%s'",
                $subject
            ));
        }
    }

    /**
     * Returns matcher priority.
     *
     * @return integer
     */
    public function getPriority()
    {
        return 0;
    }
}