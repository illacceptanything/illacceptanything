<?php

namespace spec\PhpSpec\Console;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;
use PhpSpec\Event\ExampleEvent;

class ResultConverterSpec extends ObjectBehavior
{
    function it_converts_passed_result_code_into_0()
    {
        $this->convert(ExampleEvent::PASSED)->shouldReturn(0);
    }

    function it_converts_skipped_result_code_into_0()
    {
        $this->convert(ExampleEvent::SKIPPED)->shouldReturn(0);
    }

    function it_converts_pending_result_code_into_1()
    {
        $this->convert(ExampleEvent::PENDING)->shouldReturn(1);
    }

    function it_converts_failed_result_code_into_1()
    {
        $this->convert(ExampleEvent::FAILED)->shouldReturn(1);
    }

    function it_converts_broken_result_code_into_1()
    {
        $this->convert(ExampleEvent::BROKEN)->shouldReturn(1);
    }
}
