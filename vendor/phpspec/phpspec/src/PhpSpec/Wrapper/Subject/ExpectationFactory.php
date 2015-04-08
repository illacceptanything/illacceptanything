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

namespace PhpSpec\Wrapper\Subject;

use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\Matcher\MatcherInterface;
use PhpSpec\Wrapper\Subject\Expectation\ConstructorDecorator;
use PhpSpec\Wrapper\Subject\Expectation\DispatcherDecorator;
use PhpSpec\Wrapper\Subject\Expectation\ExpectationInterface;
use PhpSpec\Wrapper\Subject\Expectation\UnwrapDecorator;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use PhpSpec\Runner\MatcherManager;
use PhpSpec\Wrapper\Unwrapper;

class ExpectationFactory
{
    /**
     * @var \PhpSpec\Loader\Node\ExampleNode
     */
    private $example;
    /**
     * @var \Symfony\Component\EventDispatcher\EventDispatcherInterface
     */
    private $dispatcher;
    /**
     * @var \PhpSpec\Runner\MatcherManager
     */
    private $matchers;

    /**
     * @param ExampleNode              $example
     * @param EventDispatcherInterface $dispatcher
     * @param MatcherManager           $matchers
     */
    public function __construct(ExampleNode $example, EventDispatcherInterface $dispatcher, MatcherManager $matchers)
    {
        $this->example = $example;
        $this->dispatcher = $dispatcher;
        $this->matchers = $matchers;
    }

    /**
     * @param string $expectation
     * @param mixed  $subject
     * @param array  $arguments
     *
     * @return ExpectationInterface
     */
    public function create($expectation, $subject, array $arguments = array())
    {
        if (0 === strpos($expectation, 'shouldNot')) {
            return $this->createNegative(lcfirst(substr($expectation, 9)), $subject, $arguments);
        }

        if (0 === strpos($expectation, 'should')) {
            return $this->createPositive(lcfirst(substr($expectation, 6)), $subject, $arguments);
        }
    }

    /**
     * @param string $name
     * @param mixed  $subject
     * @param array  $arguments
     *
     * @return ExpectationInterface
     */
    private function createPositive($name, $subject, array $arguments = array())
    {
        if (strtolower($name) === 'throw') {
            return $this->createDecoratedExpectation("PositiveThrow", $name, $subject, $arguments);
        }

        return $this->createDecoratedExpectation("Positive", $name, $subject, $arguments);
    }

    /**
     * @param string $name
     * @param mixed  $subject
     * @param array  $arguments
     *
     * @return ExpectationInterface
     */
    private function createNegative($name, $subject, array $arguments = array())
    {
        if (strtolower($name) === 'throw') {
            return $this->createDecoratedExpectation("NegativeThrow", $name, $subject, $arguments);
        }

        return $this->createDecoratedExpectation("Negative", $name, $subject, $arguments);
    }

    /**
     * @param string $expectation
     * @param string $name
     * @param mixed  $subject
     * @param array  $arguments
     *
     * @return ExpectationInterface
     */
    private function createDecoratedExpectation($expectation, $name, $subject, array $arguments)
    {
        $matcher = $this->findMatcher($name, $subject, $arguments);
        $expectation = "\\PhpSpec\\Wrapper\\Subject\\Expectation\\".$expectation;

        $expectation = new $expectation($matcher);

        if ($expectation instanceof Expectation\ThrowExpectation) {
            return $expectation;
        }

        return $this->decoratedExpectation($expectation, $matcher);
    }

    /**
     * @param string $name
     * @param mixed  $subject
     * @param array  $arguments
     *
     * @return MatcherInterface
     */
    private function findMatcher($name, $subject, array $arguments = array())
    {
        $unwrapper = new Unwrapper();
        $arguments = $unwrapper->unwrapAll($arguments);

        return $this->matchers->find($name, $subject, $arguments);
    }

    /**
     * @param ExpectationInterface $expectation
     * @param MatcherInterface     $matcher
     *
     * @return ConstructorDecorator
     */
    private function decoratedExpectation(ExpectationInterface $expectation, MatcherInterface $matcher)
    {
        $dispatcherDecorator = new DispatcherDecorator($expectation, $this->dispatcher, $matcher, $this->example);
        $unwrapperDecorator = new UnwrapDecorator($dispatcherDecorator, new Unwrapper());
        $constructorDecorator = new ConstructorDecorator($unwrapperDecorator);

        return $constructorDecorator;
    }
}
