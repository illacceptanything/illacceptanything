<?php

namespace spec\PhpSpec\Exception\Example;

use PhpSpec\ObjectBehavior;

class NotEqualExceptionSpec extends ObjectBehavior
{
    function let()
    {
        $this->beConstructedWith('Not equal', 2, 5);
    }

    function it_is_failure()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Exception\Example\FailureException');
    }

    function it_provides_a_link_to_expected()
    {
        $this->getExpected()->shouldReturn(2);
    }

    function it_provides_a_link_to_actual()
    {
        $this->getActual()->shouldReturn(5);
    }
}
