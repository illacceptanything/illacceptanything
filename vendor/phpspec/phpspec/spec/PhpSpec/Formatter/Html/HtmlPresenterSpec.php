<?php

namespace spec\PhpSpec\Formatter\Html;

use PhpSpec\Formatter\Presenter\Differ\Differ;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Exception\Exception;

class HtmlPresenterSpec extends ObjectBehavior
{
    function let(Differ $differ)
    {
        $this->beConstructedWith($differ);
    }

    function it_is_initializable()
    {
        $this->shouldImplement('PhpSpec\Formatter\Presenter\PresenterInterface');
    }

    function it_presents_the_code_around_where_exception_was_thrown(Exception $e)
    {
        $e->getCause()->willReturn(new \ReflectionClass($this));
        $this->presentException($e, true);
    }
}
