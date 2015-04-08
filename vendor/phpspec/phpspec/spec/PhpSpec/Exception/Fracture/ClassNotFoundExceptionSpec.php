<?php

namespace spec\PhpSpec\Exception\Fracture;

use PhpSpec\ObjectBehavior;

class ClassNotFoundExceptionSpec extends ObjectBehavior
{
    function let()
    {
        $this->beConstructedWith('Not equal', 'stdClass');
    }

    function it_is_fracture()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Exception\Fracture\FractureException');
    }

    function it_provides_a_link_to_classname()
    {
        $this->getClassname()->shouldReturn('stdClass');
    }
}
