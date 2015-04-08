<?php

namespace spec\PhpSpec\Formatter\Presenter;

use PhpSpec\ObjectBehavior;
use PhpSpec\Formatter\Presenter\Differ\Differ;

class TaggedPresenterSpec extends ObjectBehavior
{
    function let(Differ $differ)
    {
        $this->beConstructedWith($differ);
    }

    function it_wraps_value_into_tags()
    {
        $this->presentValue('string')->shouldReturn('<value>"string"</value>');
    }

    function it_wraps_string_into_tags()
    {
        $this->presentString('string')->shouldReturn('<value>string</value>');
    }
}
