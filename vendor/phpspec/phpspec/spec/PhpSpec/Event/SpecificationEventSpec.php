<?php

namespace spec\PhpSpec\Event;

use PhpSpec\ObjectBehavior;

use PhpSpec\Event\ExampleEvent as Example;
use PhpSpec\Loader\Node\SpecificationNode;
use PhpSpec\Loader\Suite;

class SpecificationEventSpec extends ObjectBehavior
{
    function let(Suite $suite, SpecificationNode $specification)
    {
        $this->beConstructedWith($specification, 10, Example::FAILED);

        $specification->getSuite()->willReturn($suite);
    }

    function it_is_an_event()
    {
        $this->shouldBeAnInstanceOf('Symfony\Component\EventDispatcher\Event');
        $this->shouldBeAnInstanceOf('PhpSpec\Event\EventInterface');
    }

    function it_provides_a_link_to_suite($suite)
    {
        $this->getSuite()->shouldReturn($suite);
    }

    function it_provides_a_link_to_specification($specification)
    {
        $this->getSpecification()->shouldReturn($specification);
    }

    function it_provides_a_link_to_time()
    {
        $this->getTime()->shouldReturn(10);
    }

    function it_provides_a_link_to_result()
    {
        $this->getResult()->shouldReturn(Example::FAILED);
    }
}
