<?php

namespace spec\PhpSpec\Loader\Node;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Locator\ResourceInterface;
use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\Loader\Suite;

use ReflectionClass;

class SpecificationNodeSpec extends ObjectBehavior
{
    public function let(ReflectionClass $class, ResourceInterface $resource)
    {
        $this->beConstructedWith('specification node', $class, $resource);
    }

    function it_is_countable()
    {
        $this->shouldImplement('Countable');
    }

    function it_provides_a_link_to_title()
    {
        $this->getTitle()->shouldReturn('specification node');
    }

    function it_provides_a_link_to_class($class)
    {
        $this->getClassReflection()->shouldReturn($class);
    }

    function it_provides_a_link_to_resource($resource)
    {
        $this->getResource()->shouldReturn($resource);
    }

    function it_provides_a_link_to_suite(Suite $suite)
    {
        $this->setSuite($suite);
        $this->getSuite()->shouldReturn($suite);
    }

    function it_provides_a_link_to_examples(ExampleNode $example)
    {
        $this->addExample($example);
        $this->addExample($example);
        $this->addExample($example);

        $this->getExamples()->shouldReturn(array($example, $example, $example));
    }

    function it_provides_a_count_of_examples(ExampleNode $example)
    {
        $this->addExample($example);
        $this->addExample($example);
        $this->addExample($example);

        $this->count()->shouldReturn(3);
    }
}
