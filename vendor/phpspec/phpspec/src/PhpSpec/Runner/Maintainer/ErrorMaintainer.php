<?php

/*
 * This file is part of PhpSpec, A php toolset to drive emergent
 * design by specification.
 *
 * (c) Marcello Duarte <marcello.duarte@gmail.com>
 * (c) Konstantin Kudryashov <ever.zet@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace PhpSpec\Runner\Maintainer;

use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\SpecificationInterface;
use PhpSpec\Runner\MatcherManager;
use PhpSpec\Runner\CollaboratorManager;
use PhpSpec\Exception\Example as ExampleException;

class ErrorMaintainer implements MaintainerInterface
{
    /**
     * @var integer
     */
    private $errorLevel;
    /**
     * @var callable|null
     */
    private $errorHandler;

    /**
     * @param integer $errorLevel
     */
    public function __construct($errorLevel)
    {
        $this->errorLevel = $errorLevel;
    }

    /**
     * @param ExampleNode $example
     *
     * @return bool
     */
    public function supports(ExampleNode $example)
    {
        return true;
    }

    /**
     * @param ExampleNode            $example
     * @param SpecificationInterface $context
     * @param MatcherManager         $matchers
     * @param CollaboratorManager    $collaborators
     */
    public function prepare(ExampleNode $example, SpecificationInterface $context,
                            MatcherManager $matchers, CollaboratorManager $collaborators)
    {
        $this->errorHandler = set_error_handler(array($this, 'errorHandler'), $this->errorLevel);
    }

    /**
     * @param ExampleNode            $example
     * @param SpecificationInterface $context
     * @param MatcherManager         $matchers
     * @param CollaboratorManager    $collaborators
     */
    public function teardown(ExampleNode $example, SpecificationInterface $context,
                             MatcherManager $matchers, CollaboratorManager $collaborators)
    {
        if (null !== $this->errorHandler) {
            set_error_handler($this->errorHandler);
        }
    }

    /**
     * @return int
     */
    public function getPriority()
    {
        return 999;
    }

    /**
     * Custom error handler.
     *
     * This method used as custom error handler when step is running.
     *
     * @see set_error_handler()
     *
     * @param integer $level
     * @param string  $message
     * @param string  $file
     * @param integer $line
     *
     * @return Boolean
     *
     * @throws ExampleException\ErrorException
     */
    final public function errorHandler($level, $message, $file, $line)
    {
        $regex = '/^Argument (\d)+ passed to (?:(?P<class>[\w\\\]+)::)?(\w+)\(\) must (?:be an instance of|implement interface) ([\w\\\]+),(?: instance of)? ([\w\\\]+) given/';

        if (E_RECOVERABLE_ERROR === $level && preg_match($regex, $message, $matches)) {
            $class = $matches['class'];

            if (in_array('PhpSpec\SpecificationInterface', class_implements($class))) {
                return true;
            }
        }

        if (0 !== error_reporting()) {
            throw new ExampleException\ErrorException($level, $message, $file, $line);
        }

        // error reporting turned off or more likely suppressed with @
        return false;
    }
}
