<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\Exception\Example\FailureException;

class IdentityMatcherSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter)
    {
        $presenter->presentValue(Argument::any())->willReturn('val1', 'val2');

        $this->beConstructedWith($presenter);
    }

    function it_is_a_matcher()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Matcher\MatcherInterface');
    }

    function it_responds_to_return()
    {
        $this->supports('return', '', array(''))->shouldReturn(true);
    }

    function it_responds_to_be()
    {
        $this->supports('be', '', array(''))->shouldReturn(true);
    }

    function it_responds_to_equal()
    {
        $this->supports('equal', '', array(''))->shouldReturn(true);
    }

    function it_responds_to_beEqualTo()
    {
        $this->supports('beEqualTo', '', array(''))->shouldReturn(true);
    }

    function it_matches_empty_strings()
    {
        $this->shouldNotThrow()->duringPositiveMatch('be', '', array(''));
    }

    function it_matches_not_empty_strings()
    {
        $this->shouldNotThrow()->duringPositiveMatch('be', 'chuck', array('chuck'));
    }

    function it_does_not_match_empty_string_with_emptish_values()
    {
        $this->shouldThrow(new FailureException('Expected val1, but got val2.'))
            ->duringPositiveMatch('be', '', array(false));
    }

    function it_does_not_match_zero_with_emptish_values()
    {
        $this->shouldThrow(new FailureException('Expected val1, but got val2.'))
            ->duringPositiveMatch('be', 0, array(false));
    }

    function it_does_not_match_null_with_emptish_values()
    {
        $this->shouldThrow(new FailureException('Expected val1, but got val2.'))
            ->duringPositiveMatch('be', null, array(false));
    }

    function it_does_not_match_false_with_emptish_values()
    {
        $this->shouldThrow(new FailureException('Expected val1, but got val2.'))
            ->duringPositiveMatch('be', false, array(''));
    }

    function it_does_not_match_non_empty_different_value()
    {
        $this->shouldThrow(new FailureException('Expected val1, but got val2.'))
            ->duringPositiveMatch('be', 'one', array('two'));
    }

    function it_mismatches_empty_string()
    {
        $this->shouldThrow(new FailureException('Did not expect val1, but got one.'))
            ->duringNegativeMatch('be', '', array(''));
    }

    function it_mismatches_not_empty_string($matcher)
    {
        $this->shouldThrow(new FailureException('Did not expect val1, but got one.'))
            ->duringNegativeMatch('be', 'chuck', array('chuck'));
    }

    function it_mismatches_empty_string_with_emptish_values()
    {
        $this->shouldNotThrow()->duringNegativeMatch('be', '', array(false));
    }

    function it_mismatches_zero_with_emptish_values_using_identity_operator()
    {
        $this->shouldNotThrow()->duringNegativeMatch('be', 0, array(false));
    }

    function it_mismatches_null_with_emptish_values_using_identity_operator()
    {
        $this->shouldNotThrow()->duringNegativeMatch('be', null, array(false));
    }

    function it_mismatches_false_with_emptish_values_using_identity_operator()
    {
        $this->shouldNotThrow()->duringNegativeMatch('be', false, array(''));
    }

    function it_mismatches_on_non_empty_different_value()
    {
        $this->shouldNotThrow()->duringNegativeMatch('be', 'one', array('two'));
    }
}
