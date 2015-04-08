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
use PhpSpec\Matcher\MatcherInterface;

/**
 * Class ExpectationEvent holds information about the expectation event
 */
class ExpectationEvent extends Event implements EventInterface
{
    /**
     * Expectation passed
     */
    const PASSED  = 0;

    /**
     * Expectation failed
     */
    const FAILED  = 1;

    /**
     * Expectation broken
     */
    const BROKEN  = 2;

    /**
     * @var \PhpSpec\Loader\Node\ExampleNode
     */
    private $example;

    /**
     * @var \PhpSpec\Matcher\MatcherInterface
     */
    private $matcher;

    /**
     * @var mixed
     */
    private $subject;

    /**
     * @var string
     */
    private $method;

    /**
     * @var array
     */
    private $arguments;

    /**
     * @var integer
     */
    private $result;

    /**
     * @var \Exception
     */
    private $exception;

    /**
     * @param ExampleNode      $example
     * @param MatcherInterface $matcher
     * @param mixed            $subject
     * @param string           $method
     * @param array            $arguments
     * @param integer          $result
     * @param \Exception       $exception
     */
    public function __construct(ExampleNode $example, MatcherInterface $matcher, $subject,
                                $method, $arguments, $result = null, $exception = null)
    {
        $this->example = $example;
        $this->matcher = $matcher;
        $this->subject = $subject;
        $this->method = $method;
        $this->arguments = $arguments;
        $this->result = $result;
        $this->exception = $exception;
    }

    /**
     * @return MatcherInterface
     */
    public function getMatcher()
    {
        return $this->matcher;
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
        return $this->example->getSpecification()->getSuite();
    }

    /**
     * @return mixed
     */
    public function getSubject()
    {
        return $this->subject;
    }

    /**
     * @return string
     */
    public function getMethod()
    {
        return $this->method;
    }

    /**
     * @return array
     */
    public function getArguments()
    {
        return $this->arguments;
    }

    /**
     * @return \Exception
     */
    public function getException()
    {
        return $this->exception;
    }

    /**
     * @return integer
     */
    public function getResult()
    {
        return $this->result;
    }
}
