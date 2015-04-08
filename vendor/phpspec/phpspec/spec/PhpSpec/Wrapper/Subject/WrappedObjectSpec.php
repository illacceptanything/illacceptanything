<?php

namespace spec\PhpSpec\Wrapper\Subject;

use PhpSpec\Exception\Fracture\FactoryDoesNotReturnObjectException;
use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;

class WrappedObjectSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter)
    {
        $this->beConstructedWith(null, $presenter);
    }

    function it_instantiates_object_using_classname()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('ArrayObject'));
        $this->instantiate()->shouldHaveType('ArrayObject');
    }

    function it_keeps_instantiated_object()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('ArrayObject'));
        $this->instantiate()->shouldBeEqualTo($this->getInstance());
    }

    function it_can_be_instantiated_with_a_factory_method()
    {
        $this->callOnWrappedObject(
            'beConstructedThrough',
            array(
                '\DateTime::createFromFormat',
                array('d-m-Y', '01-01-1970')
            )
        );
        $this->instantiate()->shouldHaveType('\DateTime');
    }

    function it_can_be_instantiated_with_a_factory_method_with_method_name_only()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('\DateTime'));
        $this->callOnWrappedObject(
            'beConstructedThrough',
            array(
                'createFromFormat',
                array('d-m-Y', '01-01-1970')
            )
        );
        $this->instantiate()->shouldHaveType('\DateTime');
    }

    function it_throws_an_exception_when_factory_method_returns_a_non_object()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('\DateTimeZone'));
        $this->callOnWrappedObject('beConstructedThrough', array('listAbbreviations'));

        $message = 'The method \DateTimeZone::listAbbreviations did not return an object, returned array instead';
        $this->shouldThrow(new FactoryDoesNotReturnObjectException($message))->duringInstantiate();
    }

    function it_throws_an_exception_when_trying_to_change_constructor_params_after_instantiation()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('\DateTime'));

        $this->callOnWrappedObject('beConstructedWith', array(array('now')));
        $this->callOnWrappedObject('instantiate', array());
        $this->shouldThrow('PhpSpec\Exception\Wrapper\SubjectException')->duringBeConstructedWith('tomorrow');
    }

    function it_throws_an_exception_when_trying_to_change_factory_method_after_instantiation()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('\DateTime'));

        $this->callOnWrappedObject('beConstructedThrough', array('createFromFormat',array('d-m-Y', '01-01-1980')));
        $this->callOnWrappedObject('instantiate', array());
        $this->shouldThrow('PhpSpec\Exception\Wrapper\SubjectException')
            ->duringBeConstructedThrough(array('createFromFormat',array('d-m-Y', '01-01-1970')));
    }

    function it_throws_an_exception_when_trying_to_change_from_constructor_to_factory_method_after_instantiation()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('\DateTime'));

        $this->callOnWrappedObject('beConstructedWith', array(array('now')));
        $this->callOnWrappedObject('instantiate', array());
        $this->shouldThrow('PhpSpec\Exception\Wrapper\SubjectException')
            ->duringBeConstructedThrough(array('createFromFormat',array('d-m-Y', '01-01-1970')));
    }

    function it_throws_an_exception_when_trying_to_change_from_factory_method_to_constructor_after_instantiation()
    {
        $this->callOnWrappedObject('beAnInstanceOf', array('\DateTime'));

        $this->callOnWrappedObject('beConstructedThrough', array('createFromFormat',array('d-m-Y', '01-01-1980')));
        $this->callOnWrappedObject('instantiate', array());
        $this->shouldThrow('PhpSpec\Exception\Wrapper\SubjectException')->duringBeConstructedWith('tomorrow');
    }
}
