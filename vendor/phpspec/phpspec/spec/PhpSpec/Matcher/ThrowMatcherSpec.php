<?php

namespace spec\PhpSpec\Matcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Wrapper\Unwrapper;
use PhpSpec\Formatter\Presenter\PresenterInterface;

use ArrayObject;

class ThrowMatcherSpec extends ObjectBehavior
{
    function let(Unwrapper $unwrapper, PresenterInterface $presenter)
    {
        $unwrapper->unwrapAll(Argument::any())->willReturnArgument();
        $presenter->presentValue(Argument::any())->willReturn('val1', 'val2');

        $this->beConstructedWith($unwrapper, $presenter);
    }

    function it_supports_the_throw_alias_for_object_and_exception_name()
    {
        $this->supports('throw', '', array())->shouldReturn(true);
    }

    function it_accepts_a_method_during_which_an_exception_should_be_thrown(ArrayObject $arr)
    {
        $arr->ksort()->willThrow('\Exception');

        $this->positiveMatch('throw', $arr, array('\Exception'))->during('ksort', array());
    }

    function it_accepts_a_method_during_which_an_exception_should_not_be_thrown(ArrayObject $arr)
    {
        $this->negativeMatch('throw', $arr, array('\Exception'))->during('ksort', array());
    }
}
