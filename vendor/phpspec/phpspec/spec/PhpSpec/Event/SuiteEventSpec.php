<?php

namespace spec\PhpSpec\Event;

use PhpSpec\ObjectBehavior;

use PhpSpec\Event\ExampleEvent as Example;
use PhpSpec\Loader\Suite;

class SuiteEventSpec extends ObjectBehavior
{
    function let(Suite $suite)
    {
        $this->beConstructedWith($suite, 10, Example::FAILED);
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

    function it_provides_a_link_to_time()
    {
        $this->getTime()->shouldReturn(10);
    }

    function it_provides_a_link_to_result()
    {
        $this->getResult()->shouldReturn(Example::FAILED);
    }

    function it_defaults_to_saying_suite_is_not_worth_rerunning()
    {
        $this->isWorthRerunning()->shouldReturn(false);
    }

    function it_can_be_told_that_the_suite_is_worth_rerunning()
    {
        $this->markAsWorthRerunning();
        $this->isWorthRerunning()->shouldReturn(true);
    }
}
