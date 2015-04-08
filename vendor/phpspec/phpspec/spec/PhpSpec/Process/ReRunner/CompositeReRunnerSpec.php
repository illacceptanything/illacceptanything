<?php

namespace spec\PhpSpec\Process\ReRunner;

use PhpSpec\ObjectBehavior;
use PhpSpec\Process\ReRunner;
use PhpSpec\Process\ReRunner\PlatformSpecificReRunner;
use Prophecy\Argument;

class CompositeReRunnerSpec extends ObjectBehavior
{
    function let(PlatformSpecificReRunner $reRunner1, PlatformSpecificReRunner $reRunner2)
    {
        $this->beConstructedWith(
            array(
                $reRunner1->getWrappedObject(),
                $reRunner2->getWrappedObject()
            )
        );
    }

    function it_is_a_rerunner()
    {
        $this->shouldHaveType('PhpSpec\Process\ReRunner');
    }

    function it_invokes_the_first_supported_child_to_rerun_the_suite_even_if_later_children_are_supported(
        PlatformSpecificReRunner $reRunner1, PlatformSpecificReRunner $reRunner2
    ) {
        $reRunner1->isSupported()->willReturn(true);
        $reRunner2->isSupported()->willReturn(true);

        $reRunner1->reRunSuite()->shouldBeCalled();

        $this->reRunSuite();

        $reRunner1->reRunSuite()->shouldHaveBeenCalled();
        $reRunner2->reRunSuite()->shouldNotHaveBeenCalled();
    }

    function it_skips_early_child_if_it_is_not_supported_and_invokes_runsuite_on_later_supported_child(
        PlatformSpecificReRunner $reRunner1, PlatformSpecificReRunner $reRunner2
    ) {
        $reRunner1->isSupported()->willReturn(false);
        $reRunner2->isSupported()->willReturn(true);

        $reRunner2->reRunSuite()->willReturn();

        $this->reRunSuite();

        $reRunner1->reRunSuite()->shouldNotHaveBeenCalled();
        $reRunner2->reRunSuite()->shouldHaveBeenCalled();
    }
}
