<?php

namespace spec\PhpSpec\Formatter\Html;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\IO\IOInterface;

class TemplateSpec extends ObjectBehavior
{
    function let(IOInterface $io)
    {
        $this->beConstructedWith($io);
    }

    function it_renders_the_string_as_is($io)
    {
        $this->render('text');

        $io->write('text')->shouldHaveBeenCalled();
    }

    function it_renders_a_variable($io)
    {
        $this->render('hello {name}', array('name' => 'Chuck Norris'));
        $io->write('hello Chuck Norris')->shouldHaveBeenCalled();
    }

    function it_works_for_many_instances_of_vars($io)
    {
        $this->render('{name}! {greeting}, {name}', array(
            'name' => 'Chuck',
            'greeting' => 'hello'
        ));
        $io->write('Chuck! hello, Chuck')->shouldHaveBeenCalled();
    }

    function it_renders_a_file($io)
    {
        $tempFile = __DIR__."/_files/TemplateRenderFixture.tpl";
        mkdir(__DIR__."/_files");
        file_put_contents($tempFile, 'hello, {name}');

        $this->render($tempFile, array('name' => 'Chuck'));

        $io->write('hello, Chuck')->shouldHaveBeenCalled();
    }

    function letgo()
    {
        if (file_exists(__DIR__."/_files/TemplateRenderFixture.tpl")) {
            unlink(__DIR__."/_files/TemplateRenderFixture.tpl");
            rmdir(__DIR__."/_files");
        }
    }
}
