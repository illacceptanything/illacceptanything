<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;

class StringEndMatcherSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter)
    {
        $presenter->presentString(Argument::type('string'))->willReturnArgument();

        $this->beConstructedWith($presenter);
    }

    function it_is_a_matcher()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Matcher\MatcherInterface');
    }

    function it_supports_endWith_keyword_and_string_subject()
    {
        $this->supports('endWith', 'hello, everzet', array('everzet'))->shouldReturn(true);
    }

    function it_does_not_support_anything_else()
    {
        $this->supports('endWith', array(), array())->shouldReturn(false);
    }

    function it_matches_strings_that_start_with_specified_prefix()
    {
        $this->shouldNotThrow()->duringPositiveMatch('endWith', 'everzet', array('zet'));
    }

    function it_does_not_match_strings_that_do_not_start_with_specified_prefix()
    {
        $this->shouldThrow()->duringPositiveMatch('endWith', 'everzet', array('tez'));
    }

    function it_matches_strings_that_do_not_start_with_specified_prefix()
    {
        $this->shouldNotThrow()->duringNegativeMatch('endWith', 'everzet', array('tez'));
    }

    function it_does_not_match_strings_that_do_start_with_specified_prefix()
    {
        $this->shouldThrow()->duringNegativeMatch('endWith', 'everzet', array('zet'));
    }
}
