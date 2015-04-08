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

namespace PhpSpec\Event;

use Symfony\Component\EventDispatcher\Event;
use PhpSpec\Loader\Node\ExampleNode;

/**
 * Class ExampleEvent holds the information about the example event
 */
class ExampleEvent extends Event implements EventInterface
{
    /**
     * Spec passed
     */
    const PASSED  = 0;

    /**
     * Spec is pending
     */
    const PENDING = 1;

    /**
     * Spec is skipped
     */
    const SKIPPED = 2;

    /**
     * Spec failed
     */
    const FAILED  = 3;

    /**
     * Spec is broken
     */
    const BROKEN  = 4;

    /**
     * @var \PhpSpec\Loader\Node\ExampleNode
     */
    private $example;

    /**
     * @var float
     */
    private $time;

    /**
     * @var integer
     */
    private $result;

    /**
     * @var \Exception
     */
    private $exception;

    /**
     * @param ExampleNode  $example
     * @param float|null   $time
     * @param integer|null $result
     * @param \Exception   $exception
     */
    public function __construct(ExampleNode $example, $time = null, $result = null,
                                \Exception $exception = null)
    {
        $this->example   = $example;
        $this->time      = $time;
        $this->result    = $result;
        $this->exception = $exception;
    }

    /**
     * @return ExampleNode
     */
    public function getExample()
    {
        return $this->example;
    }

    /**
     * @return \PhpSpec\Loader\Node\SpecificationNode
     */
    public function getSpecification()
    {
        return $this->example->getSpecification();
    }

    /**
     * @return \PhpSpec\Loader\Suite
     */
    public function getSuite()
    {
        return $this->getSpecification()->getSuite();
    }

    /**
     * @return string
     */
    public function getTitle()
    {
        return $this->example->getTitle();
    }

    /**
     * @return string
     */
    public function getMessage()
    {
        return $this->exception->getMessage();
    }

    /**
     * @return array
     */
    public function getBacktrace()
    {
        return $this->exception->getTrace();
    }

    /**
     * @return float
     */
    public function getTime()
    {
        return $this->time;
    }

    /**
     * @return integer
     */
    public function getResult()
    {
        return $this->result;
    }

    /**
     * @return \Exception
     */
    public function getException()
    {
        return $this->exception;
    }
}
