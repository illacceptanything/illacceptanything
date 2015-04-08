<?php

namespace spec\PhpSpec\Wrapper\Subject;

use PhpSpec\Matcher\MatcherInterface;
use PhpSpec\ObjectBehavior;
use PhpSpec\Wrapper\Subject;
use Prophecy\Argument;

use PhpSpec\Loader\Node\ExampleNode;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use PhpSpec\Runner\MatcherManager;

class ExpectationFactorySpec extends ObjectBehavior
{
    function let(ExampleNode $example, EventDispatcherInterface $dispatcher, MatcherManager $matchers)
    {
        $this->beConstructedWith($example, $dispatcher, $matchers);
    }

    function it_creates_positive_expectations(MatcherManager $matchers, MatcherInterface $matcher, Subject $subject)
    {
        $matchers->find(Argument::cetera())->willReturn($matcher);

        $subject->__call('getWrappedObject', array())->willReturn(new \stdClass());
        $decoratedExpecation = $this->create('shouldBe', $subject);

        $decoratedExpecation->shouldHaveType('PhpSpec\Wrapper\Subject\Expectation\Decorator');
        $decoratedExpecation->getNestedExpectation()->shouldHaveType('PhpSpec\Wrapper\Subject\Expectation\Positive');
    }

    function it_creates_negative_expectations(MatcherManager $matchers, MatcherInterface $matcher, Subject $subject)
    {
        $matchers->find(Argument::cetera())->willReturn($matcher);

        $subject->__call('getWrappedObject', array())->willReturn(new \stdClass());
        $decoratedExpecation = $this->create('shouldNotbe', $subject);

        $decoratedExpecation->shouldHaveType('PhpSpec\Wrapper\Subject\Expectation\Decorator');
        $decoratedExpecation->getNestedExpectation()->shouldHaveType('PhpSpec\Wrapper\Subject\Expectation\Negative');
    }

    function it_creates_positive_throw_expectations(MatcherManager $matchers, MatcherInterface $matcher, Subject $subject)
    {
        $matchers->find(Argument::cetera())->willReturn($matcher);

        $subject->__call('getWrappedObject', array())->willReturn(new \stdClass());
        $expectation = $this->create('shouldThrow', $subject);

        $expectation->shouldHaveType('PhpSpec\Wrapper\Subject\Expectation\PositiveThrow');
    }

    function it_creates_negative_throw_expectations(MatcherManager $matchers, MatcherInterface $matcher, Subject $subject)
    {
        $matchers->find(Argument::cetera())->willReturn($matcher);

        $subject->__call('getWrappedObject', array())->willReturn(new \stdClass());
        $expectation = $this->create('shouldNotThrow', $subject);

        $expectation->shouldHaveType('PhpSpec\Wrapper\Subject\Expectation\NegativeThrow');
    }
}
