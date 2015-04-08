<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\Exception\Example\FailureException;

use ArrayObject;

class ArrayCountMatcherSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter)
    {
        $presenter->presentValue(Argument::any())->willReturn('countable');
        $presenter->presentString(Argument::any())->willReturnArgument();

        $this->beConstructedWith($presenter);
    }

    function it_is_a_matcher()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Matcher\MatcherInterface');
    }

    function it_responds_to_haveCount()
    {
        $this->supports('haveCount', array(), array(''))->shouldReturn(true);
    }

    function it_matches_proper_array_count()
    {
        $this->shouldNotThrow()->duringPositiveMatch('haveCount', array(1,2,3), array(3));
    }

    function it_matches_proper_countable_count(ArrayObject $countable)
    {
        $countable->count()->willReturn(4);

        $this->shouldNotThrow()->duringPositiveMatch('haveCount', $countable, array(4));
    }

    function it_does_not_match_wrong_array_count()
    {
        $this->shouldThrow(new FailureException('Expected countable to have 2 items, but got 3.'))
            ->duringPositiveMatch('haveCount', array(1,2,3), array(2));
    }

    function it_does_not_match_proper_countable_count(ArrayObject $countable)
    {
        $countable->count()->willReturn(5);

        $this->shouldThrow(new FailureException('Expected countable to have 4 items, but got 5.'))
            ->duringPositiveMatch('haveCount', $countable, array(4));
    }

    function it_mismatches_wrong_array_count()
    {
        $this->shouldNotThrow()->duringNegativeMatch('haveCount', array(1,2,3), array(2));
    }

    function it_mismatches_wrong_countable_count(ArrayObject $countable)
    {
        $countable->count()->willReturn(5);

        $this->shouldNotThrow()->duringNegativeMatch('haveCount', $countable, array(4));
    }
}
