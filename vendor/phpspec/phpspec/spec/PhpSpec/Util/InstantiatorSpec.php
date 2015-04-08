<?php

namespace spec\PhpSpec\Util;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class InstantiatorSpec extends ObjectBehavior
{
    function it_creates_an_instance()
    {
        $this->instantiate('spec\PhpSpec\Util\NoConstructor')
            ->shouldBeAnInstanceOf('spec\PhpSpec\Util\NoConstructor');
    }

    function it_creates_an_instance_ignoring_constructor()
    {
        $this->instantiate('spec\PhpSpec\Util\WithConstructor')
            ->shouldBeAnInstanceOf('spec\PhpSpec\Util\WithConstructor');
    }

    function it_creates_an_instance_with_properties()
    {
        $this->instantiate('spec\PhpSpec\Util\WithProperties')
            ->shouldBeAnInstanceOf('spec\PhpSpec\Util\WithProperties');
    }

    function it_complains_if_class_does_not_exist()
    {
        $this->shouldThrow('PhpSpec\Exception\Fracture\ClassNotFoundException')
            ->duringInstantiate('NonExistingClass');
    }
}

class NoConstructor
{
}

class WithConstructor
{
    public function __construct($requiredArgument)
    {
    }
}

class WithProperties
{
    private $foo;

    protected $bar;

    public $baz;
}
