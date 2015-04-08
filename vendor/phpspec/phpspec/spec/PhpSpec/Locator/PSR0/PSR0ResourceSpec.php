<?php

namespace spec\PhpSpec\Locator\PSR0;

use PhpSpec\ObjectBehavior;

use PhpSpec\Locator\PSR0\PSR0Locator as Locator;

class PSR0ResourceSpec extends ObjectBehavior
{
    function let(Locator $locator)
    {
        $this->beConstructedWith(array('usr', 'lib', 'config'), $locator);
    }

    function it_uses_last_segment_as_name()
    {
        $this->getName()->shouldReturn('config');
    }

    function it_uses_last_segment_plus_Spec_suffix_as_specName()
    {
        $this->getSpecName()->shouldReturn('configSpec');
    }

    function it_is_a_resource()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Locator\ResourceInterface');
    }

    function it_generates_src_filename_from_provided_parts_using_locator($locator)
    {
        $locator->getFullSrcPath()->willReturn('/local/');

        $this->getSrcFilename()->shouldReturn('/local/usr'.DIRECTORY_SEPARATOR.'lib'.DIRECTORY_SEPARATOR.'config.php');
    }

    function it_generates_src_namespace_from_provided_parts_using_locator($locator)
    {
        $locator->getSrcNamespace()->willReturn('Local\\');

        $this->getSrcNamespace()->shouldReturn('Local\usr\lib');
    }

    function it_generates_proper_src_namespace_even_if_there_is_only_one_part($locator)
    {
        $this->beConstructedWith(array('config'), $locator);
        $locator->getSrcNamespace()->willReturn('Local\\');

        $this->getSrcNamespace()->shouldReturn('Local');
    }

    function it_generates_src_classname_from_provided_parts_using_locator($locator)
    {
        $locator->getSrcNamespace()->willReturn('Local\\');

        $this->getSrcClassname()->shouldReturn('Local\usr\lib\config');
    }

    function it_generates_proper_src_classname_for_empty_locator_namespace($locator)
    {
        $locator->getSrcNamespace()->willReturn('');

        $this->getSrcClassname()->shouldReturn('usr\lib\config');
    }

    function it_generates_spec_filename_from_provided_parts_using_locator($locator)
    {
        $locator->getFullSpecPath()->willReturn('/local/spec/');

        $this->getSpecFilename()->shouldReturn('/local/spec/usr'.DIRECTORY_SEPARATOR.'lib'.DIRECTORY_SEPARATOR.'configSpec.php');
    }

    function it_generates_spec_namespace_from_provided_parts_using_locator($locator)
    {
        $locator->getSpecNamespace()->willReturn('spec\Local\\');

        $this->getSpecNamespace()->shouldReturn('spec\Local\usr\lib');
    }

    function it_generates_proper_spec_namespace_even_if_there_is_only_one_part($locator)
    {
        $this->beConstructedWith(array('config'), $locator);
        $locator->getSpecNamespace()->willReturn('spec\Local\\');

        $this->getSpecNamespace()->shouldReturn('spec\Local');
    }

    function it_generates_spec_classname_from_provided_parts_using_locator($locator)
    {
        $locator->getSpecNamespace()->willReturn('spec\Local\\');

        $this->getSpecClassname()->shouldReturn('spec\Local\usr\lib\configSpec');
    }
}
