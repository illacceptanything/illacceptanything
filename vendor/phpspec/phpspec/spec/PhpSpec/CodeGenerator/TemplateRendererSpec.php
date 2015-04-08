<?php

namespace spec\PhpSpec\CodeGenerator;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Util\Filesystem;

class TemplateRendererSpec extends ObjectBehavior
{
    function let(Filesystem $fs)
    {
        $this->beConstructedWith($fs);
    }

    function it_does_not_have_registered_locations_by_default()
    {
        $this->getLocations()->shouldHaveCount(0);
    }

    function it_has_locations_setter()
    {
        $this->setLocations(array('location1', 'location2'));
        $this->getLocations()->shouldReturn(array('location1', 'location2'));
    }

    function it_provides_a_method_to_prepend_location()
    {
        $this->setLocations(array('location1', 'location2'));
        $this->prependLocation('location0');

        $this->getLocations()->shouldReturn(array('location0', 'location1', 'location2'));
    }

    function it_provides_a_method_to_append_location()
    {
        $this->setLocations(array('location1', 'location2'));
        $this->appendLocation('location0');

        $this->getLocations()->shouldReturn(array('location1', 'location2', 'location0'));
    }

    function it_normalizes_locations()
    {
        $this->setLocations(array('lo/ca\\tion', '\\location', 'location\\'));
        $this->getLocations()->shouldReturn(array(
            'lo'.DIRECTORY_SEPARATOR.'ca'.DIRECTORY_SEPARATOR.'tion',
            DIRECTORY_SEPARATOR.'location',
            'location'
        ));
    }

    function it_reads_existing_file_from_registered_location($fs)
    {
        $fs->pathExists('location1'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn(true);
        $fs->getFileContents('location1'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn('cont');

        $this->setLocations(array('location1'));
        $this->render('some_file')->shouldReturn('cont');
    }

    function it_reads_existing_file_from_first_registered_location($fs)
    {
        $fs->pathExists('location1'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn(false);
        $fs->pathExists('location2'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn(true);
        $fs->pathExists('location3'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn(true);
        $fs->getFileContents('location2'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn('cont');
        $fs->getFileContents('location3'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn('cont2');

        $this->setLocations(array('location1', 'location2', 'location3'));
        $this->render('some_file')->shouldReturn('cont');
    }

    function it_replaces_placeholders_in_template_with_provided_values($fs)
    {
        $fs->pathExists('location1'.DIRECTORY_SEPARATOR.'some_file.tpl')->willReturn(true);
        $fs->getFileContents('location1'.DIRECTORY_SEPARATOR.'some_file.tpl')
            ->willReturn('Template #%number%. From %spec_name% spec.');

        $this->setLocations(array('location1'));
        $this->render('some_file', array('%number%' => 2, '%spec_name%' => 'tpl'))
            ->shouldReturn('Template #2. From tpl spec.');
    }

    function it_can_render_template_from_string()
    {
        $this->renderString('Template #%number%. From %spec_name% spec.', array(
            '%number%'    => 2,
            '%spec_name%' => 'tpl'
        ))->shouldReturn('Template #2. From tpl spec.');
    }

    function it_returns_null_if_template_is_not_found_in_any_registered_locations()
    {
        $this->render('some_file')->shouldReturn(null);
    }
}
