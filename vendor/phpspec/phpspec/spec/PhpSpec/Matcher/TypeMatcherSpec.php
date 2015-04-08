<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\Exception\Example\FailureException;

use ArrayObject;

class TypeMatcherSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter)
    {
        $presenter->presentString(Argument::any())->willReturnArgument();
        $presenter->presentValue(Argument::any())->willReturn('object');

        $this->beConstructedWith($presenter);
    }

    function it_is_a_matcher()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Matcher\MatcherInterface');
    }

    function it_responds_to_beAnInstanceOf()
    {
        $this->supports('beAnInstanceOf', '', array(''))->shouldReturn(true);
    }

    function it_responds_to_returnAnInstanceOf()
    {
        $this->supports('returnAnInstanceOf', '', array(''))->shouldReturn(true);
    }

    function it_responds_to_haveType()
    {
        $this->supports('haveType', '', array(''))->shouldReturn(true);
    }

    function it_matches_subclass_instance(ArrayObject $object)
    {
        $this->shouldNotThrow()
            ->duringPositiveMatch('haveType', $object, array('ArrayObject'));
    }

    function it_matches_interface_instance(ArrayObject $object)
    {
        $this->shouldNotThrow()
            ->duringPositiveMatch('haveType', $object, array('ArrayAccess'));
    }

    function it_does_not_match_wrong_class(ArrayObject $object)
    {
        $this->shouldThrow(new FailureException(
            'Expected an instance of stdClass, but got object.'
        ))->duringPositiveMatch('haveType', $object, array('stdClass'));
    }

    function it_does_not_match_wrong_interface(ArrayObject $object)
    {
        $this->shouldThrow(new FailureException(
            'Expected an instance of SessionHandlerInterface, but got object.'
        ))->duringPositiveMatch('haveType', $object, array('SessionHandlerInterface'));
    }

    function it_matches_other_class(ArrayObject $object)
    {
        $this->shouldNotThrow()->duringNegativeMatch('haveType', $object, array('stdClass'));
    }

    function it_matches_other_interface()
    {
        $this->shouldNotThrow()
            ->duringNegativeMatch('haveType', $this, array('SessionHandlerInterface'));
    }
}
