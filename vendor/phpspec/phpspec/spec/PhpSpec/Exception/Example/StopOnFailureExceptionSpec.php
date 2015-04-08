<?php

namespace spec\PhpSpec\Exception\Example;

use PhpSpec\Event\ExampleEvent;
use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class StopOnFailureExceptionSpec extends ObjectBehavior
{
    function let()
    {
        $this->beConstructedWith('Message', 0, null, 1);
    }

    function it_is_an_example_exception()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Exception\Example\ExampleException');
    }

    function it_has_a_the_result_of_the_last_spec()
    {
        $this->getResult()->shouldReturn(1);
    }
}
