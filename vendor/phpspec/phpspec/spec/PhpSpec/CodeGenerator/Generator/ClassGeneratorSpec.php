<?php

namespace spec\PhpSpec\CodeGenerator\Generator;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Console\IO;
use PhpSpec\CodeGenerator\TemplateRenderer;
use PhpSpec\Util\Filesystem;
use PhpSpec\Locator\ResourceInterface;

class ClassGeneratorSpec extends ObjectBehavior
{
    function let(IO $io, TemplateRenderer $tpl, Filesystem $fs)
    {
        $this->beConstructedWith($io, $tpl, $fs);
    }

    function it_is_a_generator()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\CodeGenerator\Generator\GeneratorInterface');
    }

    function it_supports_class_generation(ResourceInterface $resource)
    {
        $this->supports($resource, 'class', array())->shouldReturn(true);
    }

    function it_does_not_support_anything_else(ResourceInterface $resource)
    {
        $this->supports($resource, 'anything_else', array())->shouldReturn(false);
    }

    function its_priority_is_0()
    {
        $this->getPriority()->shouldReturn(0);
    }

    function it_generates_class_from_resource_and_puts_it_into_appropriate_folder(
        $io, $tpl, $fs, ResourceInterface $resource
    ) {
        $resource->getName()->willReturn('App');
        $resource->getSrcFilename()->willReturn('/project/src/Acme/App.php');
        $resource->getSrcNamespace()->willReturn('Acme');
        $resource->getSrcClassname()->willReturn('Acme\App');

        $values = array(
            '%filepath%'        => '/project/src/Acme/App.php',
            '%name%'            => 'App',
            '%namespace%'       => 'Acme',
            '%namespace_block%' => "\n\nnamespace Acme;",
        );

        $tpl->render('class', $values)->willReturn(null);
        $tpl->renderString(Argument::type('string'), $values)->willReturn('generated code');

        $fs->pathExists('/project/src/Acme/App.php')->willReturn(false);
        $fs->isDirectory('/project/src/Acme')->willReturn(true);
        $fs->putFileContents('/project/src/Acme/App.php', 'generated code')->shouldBeCalled();

        $this->generate($resource);
    }

    function it_uses_template_provided_by_templating_system_if_there_is_one(
        $io, $tpl, $fs, ResourceInterface $resource
    ) {
        $resource->getName()->willReturn('App');
        $resource->getSrcFilename()->willReturn('/project/src/Acme/App.php');
        $resource->getSrcNamespace()->willReturn('Acme');
        $resource->getSrcClassname()->willReturn('Acme\App');

        $values = array(
            '%filepath%'        => '/project/src/Acme/App.php',
            '%name%'            => 'App',
            '%namespace%'       => 'Acme',
            '%namespace_block%' => "\n\nnamespace Acme;",
        );

        $tpl->render('class', $values)->willReturn('template code');
        $tpl->renderString(Argument::type('string'), $values)->willReturn('generated code');

        $fs->pathExists('/project/src/Acme/App.php')->willReturn(false);
        $fs->isDirectory('/project/src/Acme')->willReturn(true);
        $fs->putFileContents('/project/src/Acme/App.php', 'template code')->shouldBeCalled();

        $this->generate($resource);
    }

    function it_creates_folder_for_class_if_needed($io, $tpl, $fs, ResourceInterface $resource)
    {
        $resource->getName()->willReturn('App');
        $resource->getSrcFilename()->willReturn('/project/src/Acme/App.php');
        $resource->getSrcNamespace()->willReturn('Acme');
        $resource->getSrcClassname()->willReturn('Acme\App');

        $fs->pathExists('/project/src/Acme/App.php')->willReturn(false);
        $fs->isDirectory('/project/src/Acme')->willReturn(false);
        $fs->makeDirectory('/project/src/Acme')->shouldBeCalled();
        $fs->putFileContents('/project/src/Acme/App.php', Argument::any())->willReturn(null);

        $this->generate($resource);
    }

    function it_asks_confirmation_if_class_already_exists(
        $io, $tpl, $fs, ResourceInterface $resource
    ) {
        $resource->getName()->willReturn('App');
        $resource->getSrcFilename()->willReturn('/project/src/Acme/App.php');
        $resource->getSrcNamespace()->willReturn('Acme');
        $resource->getSrcClassname()->willReturn('Acme\App');

        $fs->pathExists('/project/src/Acme/App.php')->willReturn(true);
        $io->askConfirmation(Argument::type('string'), false)->willReturn(false);

        $fs->putFileContents(Argument::cetera())->shouldNotBeCalled();

        $this->generate($resource);
    }
}
