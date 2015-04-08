<?php

namespace spec\PhpSpec\Listener;

use PhpSpec\CodeGenerator\GeneratorManager;
use PhpSpec\Console\IO;
use PhpSpec\Event\ExampleEvent;
use PhpSpec\Event\SuiteEvent;
use PhpSpec\Exception\Fracture\NamedConstructorNotFoundException;
use PhpSpec\Locator\ResourceManager;
use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class NamedConstructorNotFoundListenerSpec extends ObjectBehavior
{
    function let(IO $io, ResourceManager $resourceManager, GeneratorManager $generatorManager,
                 SuiteEvent $suiteEvent, ExampleEvent $exampleEvent)
    {
        $io->writeln(Argument::any())->willReturn();
        $io->askConfirmation(Argument::any())->willReturn();

        $this->beConstructedWith($io, $resourceManager, $generatorManager);
    }

    function it_does_not_prompt_for_method_generation_if_no_exception_was_thrown($exampleEvent, $suiteEvent, $io)
    {
        $io->isCodeGenerationEnabled()->willReturn(true);

        $this->afterExample($exampleEvent);
        $this->afterSuite($suiteEvent);

        $io->askConfirmation(Argument::any())->shouldNotBeenCalled();
    }

    function it_does_not_prompt_for_method_generation_if_non_namedconstructornotfoundexception_was_thrown($exampleEvent, $suiteEvent, $io, \InvalidArgumentException $exception)
    {
        $exampleEvent->getException()->willReturn($exception);
        $io->isCodeGenerationEnabled()->willReturn(true);

        $this->afterExample($exampleEvent);
        $this->afterSuite($suiteEvent);

        $io->askConfirmation(Argument::any())->shouldNotBeenCalled();
    }

    function it_prompts_for_method_generation_if_namedconstructornotfoundexception_was_thrown_and_input_is_interactive($exampleEvent, $suiteEvent, $io, NamedConstructorNotFoundException $exception)
    {
        $exampleEvent->getException()->willReturn($exception);
        $io->isCodeGenerationEnabled()->willReturn(true);

        $this->afterExample($exampleEvent);
        $this->afterSuite($suiteEvent);

        $io->askConfirmation(Argument::any())->shouldHaveBeenCalled();
    }

    function it_does_not_prompt_for_method_generation_if_input_is_not_interactive($exampleEvent, $suiteEvent, $io, NamedConstructorNotFoundException $exception)
    {
        $exampleEvent->getException()->willReturn($exception);
        $io->isCodeGenerationEnabled()->willReturn(false);

        $this->afterExample($exampleEvent);
        $this->afterSuite($suiteEvent);

        $io->askConfirmation(Argument::any())->shouldNotBeenCalled();
    }
}
