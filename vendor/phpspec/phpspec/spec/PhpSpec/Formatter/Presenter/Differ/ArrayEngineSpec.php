<?php

namespace spec\PhpSpec\Formatter\Presenter\Differ;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class ArrayEngineSpec extends ObjectBehavior
{
    function it_is_a_diff_engine()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Formatter\Presenter\Differ\EngineInterface');
    }

    function it_supports_arrays()
    {
        $this->supports(array(), array(1, 2, 3))->shouldReturn(true);
    }

    function it_does_not_support_anything_else()
    {
        $this->supports('str', 2)->shouldReturn(false);
    }
}
