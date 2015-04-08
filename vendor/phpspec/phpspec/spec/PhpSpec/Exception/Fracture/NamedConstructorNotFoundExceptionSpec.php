<?php

namespace spec\PhpSpec\Exception\Fracture;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class NamedConstructorNotFoundExceptionSpec extends ObjectBehavior
{
    function let($subject)
    {
        $this->beConstructedWith('No named constructor', $subject, 'setName', array('jmurphy'));
    }

    function it_is_fracture()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Exception\Fracture\FractureException');
    }

    function it_provides_a_link_to_subject($subject)
    {
        $this->getSubject()->shouldReturn($subject);
    }

    function it_provides_a_link_to_methodName()
    {
        $this->getMethodName()->shouldReturn('setName');
    }

    function it_provides_a_link_to_arguments()
    {
        $this->getArguments()->shouldReturn(array('jmurphy'));
    }
}
