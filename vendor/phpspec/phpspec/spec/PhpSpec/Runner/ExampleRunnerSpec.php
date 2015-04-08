<?php

namespace spec\PhpSpec\Runner;

use PhpSpec\ObjectBehavior;
use PhpSpec\Runner\Maintainer\LetAndLetgoMaintainer;
use Prophecy\Argument;

use Symfony\Component\EventDispatcher\EventDispatcherInterface;

use PhpSpec\SpecificationInterface;
use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\Runner\Maintainer\MaintainerInterface;
use PhpSpec\Loader\Node\SpecificationNode;
use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\Event\ExampleEvent;

use ReflectionClass;
use ReflectionMethod;

class ExampleRunnerSpec extends ObjectBehavior
{
    function let(EventDispatcherInterface $dispatcher, PresenterInterface $presenter, ExampleNode $example, SpecificationNode $specification, ReflectionClass $specReflection,
        ReflectionMethod $exampReflection, SpecificationInterface $context)
    {
        $this->beConstructedWith($dispatcher, $presenter);

        $example->getSpecification()->willReturn($specification);
        $example->getFunctionReflection()->willReturn($exampReflection);
        $specification->getClassReflection()->willReturn($specReflection);
        $specReflection->newInstance()->willReturn($context);
    }

    function it_executes_example_in_newly_created_context(
        ExampleNode $example, ReflectionMethod $exampReflection, SpecificationInterface $context
    ) {
        $example->isPending()->willReturn(false);

        $exampReflection->getParameters()->willReturn(array());
        $exampReflection->invokeArgs($context, array())->shouldBeCalled();

        $this->run($example);
    }

    function it_dispatches_ExampleEvent_with_pending_status_if_example_is_pending(
        EventDispatcherInterface $dispatcher, ExampleNode $example
    ) {
        $example->isPending()->willReturn(true);

        $dispatcher->dispatch('beforeExample', Argument::any())->shouldBeCalled();
        $dispatcher->dispatch('afterExample',
            Argument::which('getResult', ExampleEvent::PENDING)
        )->shouldBeCalled();

        $this->run($example);
    }

    function it_dispatches_ExampleEvent_with_failed_status_if_matcher_throws_exception(
        EventDispatcherInterface $dispatcher,
        ExampleNode $example, ReflectionMethod $exampReflection, SpecificationInterface $context
    ) {
        $example->isPending()->willReturn(false);

        $exampReflection->getParameters()->willReturn(array());
        $exampReflection->invokeArgs($context, array())
            ->willThrow('PhpSpec\Exception\Example\FailureException');

        $dispatcher->dispatch('beforeExample', Argument::any())->shouldBeCalled();
        $dispatcher->dispatch('afterExample',
            Argument::which('getResult', ExampleEvent::FAILED)
        )->shouldBeCalled();

        $this->run($example);
    }

    function it_dispatches_ExampleEvent_with_failed_status_if_example_throws_exception(
        EventDispatcherInterface $dispatcher,
        ExampleNode $example, ReflectionMethod $exampReflection, SpecificationInterface $context
    ) {
        $example->isPending()->willReturn(false);

        $exampReflection->getParameters()->willReturn(array());
        $exampReflection->invokeArgs($context, array())->willThrow('RuntimeException');

        $dispatcher->dispatch('beforeExample', Argument::any())->shouldBeCalled();
        $dispatcher->dispatch('afterExample',
            Argument::which('getResult', ExampleEvent::BROKEN)
        )->shouldBeCalled();

        $this->run($example);
    }

    function it_runs_all_supported_maintainers_before_and_after_each_example(
        ExampleNode $example, ReflectionMethod $exampReflection, MaintainerInterface $maintainer
    ) {
        $example->isPending()->willReturn(false);

        $exampReflection->getParameters()->willReturn(array());
        $exampReflection->invokeArgs(Argument::cetera())->willReturn(null);

        $maintainer->getPriority()->willReturn(1);
        $maintainer->supports($example)->willReturn(true);

        $maintainer->prepare($example, Argument::cetera())->shouldBeCalled();
        $maintainer->teardown($example, Argument::cetera())->shouldBeCalled();

        $this->registerMaintainer($maintainer);
        $this->run($example);
    }

    function it_runs_let_and_letgo_maintainer_before_and_after_each_example_if_the_example_throws_an_exception(
        ExampleNode $example, SpecificationNode $specification, ReflectionClass $specReflection,
        ReflectionMethod $exampReflection, LetAndLetgoMaintainer $maintainer,
        SpecificationInterface $context
    ) {
        $example->isPending()->willReturn(false);
        $example->getFunctionReflection()->willReturn($exampReflection);
        $example->getSpecification()->willReturn($specification);
        $specification->getClassReflection()->willReturn($specReflection);
        $specReflection->newInstanceArgs()->willReturn($context);

        $exampReflection->getParameters()->willReturn(array());
        $exampReflection->invokeArgs($context, array())->willThrow('RuntimeException');

        $maintainer->getPriority()->willReturn(1);
        $maintainer->supports($example)->willReturn(true);

        $maintainer->prepare($example, Argument::cetera())->shouldBeCalled();
        $maintainer->teardown($example, Argument::cetera())->shouldBeCalled();

        $this->registerMaintainer($maintainer);
        $this->run($example);
    }
}
