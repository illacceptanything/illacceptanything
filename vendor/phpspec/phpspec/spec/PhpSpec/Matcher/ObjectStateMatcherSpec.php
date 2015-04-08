<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;

class ObjectStateMatcherSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter)
    {
        $presenter->presentValue(Argument::any())->willReturn('val1', 'val2');
        $presenter->presentString(Argument::any())->willReturnArgument();

        $this->beConstructedWith($presenter);
    }

    function it_is_a_matcher()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Matcher\MatcherInterface');
    }

    function it_infers_matcher_alias_name_from_methods_prefixed_with_is()
    {
        $subject = new \ReflectionClass($this);

        $this->supports('beAbstract', $subject, array())->shouldReturn(true);
    }

    function it_throws_exception_if_checker_method_not_found()
    {
        $subject = new \ReflectionClass($this);

        $this->shouldThrow('PhpSpec\Exception\Fracture\MethodNotFoundException')
            ->duringPositiveMatch('beSimple', $subject, array());
    }

    function it_matches_if_state_checker_returns_true()
    {
        $subject = new \ReflectionClass($this);

        $this->shouldNotThrow()->duringPositiveMatch('beUserDefined', $subject, array());
    }

    function it_does_not_match_if_state_checker_returns_false()
    {
        $subject = new \ReflectionClass($this);

        $this->shouldThrow('PhpSpec\Exception\Example\FailureException')
            ->duringPositiveMatch('beFinal', $subject, array());
    }

    function it_infers_matcher_alias_name_from_methods_prefixed_with_has()
    {
        $subject = new \ReflectionClass($this);

        $this->supports('haveProperty', $subject, array('something'))->shouldReturn(true);
    }

    function it_throws_exception_if_has_checker_method_not_found()
    {
        $subject = new \ReflectionClass($this);

        $this->shouldThrow('PhpSpec\Exception\Fracture\MethodNotFoundException')
            ->duringPositiveMatch('haveAnything', $subject, array('str'));
    }

    function it_matches_if_has_checker_returns_true()
    {
        $subject = new \ReflectionClass($this);

        $this->shouldNotThrow()->duringPositiveMatch(
            'haveMethod', $subject, array('it_matches_if_has_checker_returns_true')
        );
    }

    function it_does_not_match_if_has_state_checker_returns_false()
    {
        $subject = new \ReflectionClass($this);

        $this->shouldThrow('PhpSpec\Exception\Example\FailureException')
            ->duringPositiveMatch('haveProperty', $subject, array('other'));
    }

    function it_does_not_match_if_subject_is_callable()
    {
        $subject = function () {};

        $this->supports('beCallable', $subject, array())->shouldReturn(false);
    }
}
