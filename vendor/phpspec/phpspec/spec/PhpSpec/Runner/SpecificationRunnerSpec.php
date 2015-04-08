<?php

namespace spec\PhpSpec\Runner;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use Symfony\Component\EventDispatcher\EventDispatcherInterface;

use PhpSpec\Runner\ExampleRunner;
use PhpSpec\Loader\Node\SpecificationNode;
use PhpSpec\Loader\Node\ExampleNode;

class SpecificationRunnerSpec extends ObjectBehavior
{
    function let(EventDispatcherInterface $dispatcher, ExampleRunner $exampleRunner)
    {
        $this->beConstructedWith($dispatcher, $exampleRunner);
    }

    function it_passes_each_specification_example_to_ExampleRunner(
        SpecificationNode $specification, ExampleNode $ex1, ExampleNode $ex2,
        ExampleRunner $exampleRunner
    ) {
        $specification->getExamples()->willReturn(array($ex1, $ex2));

        $exampleRunner->run($ex1)->shouldBeCalled();
        $exampleRunner->run($ex2)->shouldBeCalled();

        $this->run($specification);
    }

    function it_returns_examples_max_resultCode(
        SpecificationNode $specification, ExampleNode $ex1, ExampleNode $ex2,
        ExampleRunner $exampleRunner
    ) {
        $specification->getExamples()->willReturn(array($ex1, $ex2));

        $exampleRunner->run($ex1)->willReturn(2);
        $exampleRunner->run($ex2)->willReturn(0);

        $this->run($specification)->shouldReturn(2);
    }

    function it_returns_0_resultCode_if_no_examples_found(SpecificationNode $specification)
    {
        $specification->getExamples()->willReturn(array());

        $this->run($specification)->shouldReturn(0);
    }

    function it_dispatches_SpecificationEvent_before_and_after_examples_run(
        EventDispatcherInterface $dispatcher, SpecificationNode $specification
    ) {
        $specification->getExamples()->willReturn(array());

        $dispatcher->dispatch('beforeSpecification',
            Argument::type('PhpSpec\Event\SpecificationEvent')
        )->shouldBeCalled();

        $dispatcher->dispatch('afterSpecification',
            Argument::type('PhpSpec\Event\SpecificationEvent')
        )->shouldBeCalled();

        $this->run($specification);
    }
}
