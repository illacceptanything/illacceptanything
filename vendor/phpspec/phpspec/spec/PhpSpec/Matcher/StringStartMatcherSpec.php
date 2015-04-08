<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;

class StringStartMatcherSpec extends ObjectBehavior
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

    function it_supports_startWith_keyword_and_string_subject()
    {
        $this->supports('startWith', 'hello, everzet', array('hello'))->shouldReturn(true);
    }

    function it_does_not_support_anything_else()
    {
        $this->supports('startWith', array(), array())->shouldReturn(false);
    }

    function it_matches_strings_that_start_with_specified_prefix()
    {
        $this->shouldNotThrow()->duringPositiveMatch('startWith', 'everzet', array('ev'));
    }

    function it_does_not_match_strings_that_do_not_start_with_specified_prefix()
    {
        $this->shouldThrow()->duringPositiveMatch('startWith', 'everzet', array('av'));
    }

    function it_matches_strings_that_do_not_start_with_specified_prefix()
    {
        $this->shouldNotThrow()->duringNegativeMatch('startWith', 'everzet', array('av'));
    }

    function it_does_not_match_strings_that_do_start_with_specified_prefix()
    {
        $this->shouldThrow()->duringNegativeMatch('startWith', 'everzet', array('ev'));
    }
}
