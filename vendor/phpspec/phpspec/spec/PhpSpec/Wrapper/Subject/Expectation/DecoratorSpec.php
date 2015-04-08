<?php

namespace spec\PhpSpec\Wrapper\Subject\Expectation;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Wrapper\Subject\Expectation\Decorator as AbstractDecorator;
use PhpSpec\Wrapper\Subject\Expectation\ExpectationInterface;

class DecoratorSpec extends ObjectBehavior
{
    function let(ExpectationInterface $expectation)
    {
        $this->beAnInstanceOf('spec\PhpSpec\Wrapper\Subject\Expectation\Decorator');
        $this->beConstructedWith($expectation);
    }

    function it_returns_the_decorated_expectation(ExpectationInterface $expectation)
    {
        $this->getExpectation()->shouldReturn($expectation);
    }

    function it_keeps_looking_for_nested_expectations(AbstractDecorator $decorator, ExpectationInterface $expectation)
    {
        $decorator->getExpectation()->willReturn($expectation);
        $this->beAnInstanceOf('spec\PhpSpec\Wrapper\Subject\Expectation\Decorator');
        $this->beConstructedWith($decorator);

        $this->getNestedExpectation()->shouldReturn($expectation);
    }
}

class Decorator extends AbstractDecorator
{
    public function match($alias, $subject, array $arguments = array())
    {
    }
}
