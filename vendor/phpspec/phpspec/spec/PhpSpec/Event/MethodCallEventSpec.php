<?php

namespace spec\PhpSpec\Event;

use PhpSpec\ObjectBehavior;
use PhpSpec\Wrapper\Subject;
use PhpSpec\Loader\Suite;
use PhpSpec\Loader\Node\SpecificationNode;
use PhpSpec\Loader\Node\ExampleNode;
use Prophecy\Argument;

class MethodCallEventSpec extends ObjectBehavior
{
    function let(Suite $suite, SpecificationNode $specification, ExampleNode $example, $subject)
    {
        $method = 'calledMethod';
        $arguments = array('methodArguments');
        $returnValue = 'returned value';

        $this->beConstructedWith($example, $subject, $method, $arguments, $returnValue);

        $example->getSpecification()->willReturn($specification);
        $specification->getSuite()->willReturn($suite);
    }

    function it_is_an_event()
    {
        $this->shouldBeAnInstanceOf('Symfony\Component\EventDispatcher\Event');
        $this->shouldBeAnInstanceOf('PhpSpec\Event\EventInterface');
    }

    function it_provides_a_link_to_example($example)
    {
        $this->getExample()->shouldReturn($example);
    }

    function it_provides_a_link_to_specification($specification)
    {
        $this->getSpecification()->shouldReturn($specification);
    }

    function it_provides_a_link_to_suite($suite)
    {
        $this->getSuite()->shouldReturn($suite);
    }

    function it_provides_a_link_to_subject($subject)
    {
        $this->getSubject()->shouldReturn($subject);
    }

    function it_provides_a_link_to_method()
    {
        $this->getMethod()->shouldReturn('calledMethod');
    }

    function it_provides_a_link_to_arguments()
    {
        $this->getArguments()->shouldReturn(array('methodArguments'));
    }

    function it_provides_a_link_to_return_value()
    {
        $this->getReturnValue()->shouldReturn('returned value');
    }
}
