<?php

namespace spec\PhpSpec\Wrapper\Subject\Expectation;

use PhpSpec\Matcher\MatcherInterface;
use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class PositiveSpec extends ObjectBehavior
{
    function let(MatcherInterface $matcher)
    {
        $this->beConstructedWith($matcher);
    }

    function it_calls_a_positive_match_on_matcher(MatcherInterface $matcher)
    {
        $alias = 'somealias';
        $subject = 'subject';
        $arguments = array();

        $matcher->positiveMatch($alias, $subject, $arguments)->shouldBeCalled();
        $this->match($alias, $subject, $arguments);
    }
}
