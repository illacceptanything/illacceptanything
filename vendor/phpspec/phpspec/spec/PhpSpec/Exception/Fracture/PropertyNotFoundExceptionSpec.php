<?php

namespace spec\PhpSpec\Exception\Fracture;

use PhpSpec\ObjectBehavior;

class PropertyNotFoundExceptionSpec extends ObjectBehavior
{
    function let($subject)
    {
        $this->beConstructedWith('No method', $subject, 'attributes');
    }

    function it_is_fracture()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Exception\Fracture\FractureException');
    }

    function it_provides_a_link_to_subject($subject)
    {
        $this->getSubject()->shouldReturn($subject);
    }

    function it_provides_a_link_to_property()
    {
        $this->getProperty()->shouldReturn('attributes');
    }
}
