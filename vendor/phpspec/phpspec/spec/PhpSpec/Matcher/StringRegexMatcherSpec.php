<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;

class StringRegexMatcherSpec extends ObjectBehavior
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

    function it_supports_match_keyword_and_string_subject()
    {
        $this->supports('match', 'hello, everzet', array('/hello/'))->shouldReturn(true);
    }

    function it_does_not_support_anything_else()
    {
        $this->supports('match', array(), array())->shouldReturn(false);
    }

    function it_matches_strings_that_match_specified_regex()
    {
        $this->shouldNotThrow()->duringPositiveMatch('match', 'everzet', array('/ev.*et/'));
    }

    function it_does_not_match_strings_that_do_not_match_specified_regex()
    {
        $this->shouldThrow()->duringPositiveMatch('match', 'everzet', array('/md/'));
    }

    function it_matches_strings_that_do_not_match_specified_regex()
    {
        $this->shouldNotThrow()->duringNegativeMatch('match', 'everzet', array('/md/'));
    }

    function it_does_not_match_strings_that_do_match_specified_regex()
    {
        $this->shouldThrow()->duringNegativeMatch('match', 'everzet', array('/^ev.*et$/'));
    }
}
