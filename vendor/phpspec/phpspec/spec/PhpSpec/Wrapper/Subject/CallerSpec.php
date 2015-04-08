<?php

namespace spec\PhpSpec\Wrapper\Subject;

use PhpSpec\Exception\ExceptionFactory;
use PhpSpec\Wrapper\Subject\WrappedObject;
use PhpSpec\Wrapper\Wrapper;
use PhpSpec\Wrapper\Subject;

use PhpSpec\Loader\Node\ExampleNode;

use Symfony\Component\EventDispatcher\EventDispatcherInterface as Dispatcher;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class CallerSpec extends ObjectBehavior
{
    function let(WrappedObject $wrappedObject, ExampleNode $example,
                 Dispatcher $dispatcher, ExceptionFactory $exceptions, Wrapper $wrapper)
    {
        $this->beConstructedWith($wrappedObject, $example, $dispatcher,
            $exceptions, $wrapper);
    }

    function it_dispatches_method_call_events(Dispatcher $dispatcher, WrappedObject $wrappedObject)
    {
        $wrappedObject->isInstantiated()->willReturn(true);
        $wrappedObject->getInstance()->willReturn(new \ArrayObject());

        $dispatcher->dispatch(
            'beforeMethodCall',
            Argument::type('PhpSpec\Event\MethodCallEvent')
        )->shouldBeCalled();

        $dispatcher->dispatch(
            'afterMethodCall',
            Argument::type('PhpSpec\Event\MethodCallEvent')
        )->shouldBeCalled();

        $this->call('count');
    }

    function it_sets_a_property_on_the_wrapped_object(WrappedObject $wrappedObject)
    {
        $obj = new \stdClass();
        $obj->id = 1;

        $wrappedObject->isInstantiated()->willReturn(true);
        $wrappedObject->getInstance()->willReturn($obj);

        $this->set('id', 2)->shouldReturn(2);
    }

    function it_proxies_method_calls_to_wrapped_object(\ArrayObject $obj, WrappedObject $wrappedObject)
    {
        $obj->asort()->shouldBeCalled();
        $wrappedObject->isInstantiated()->willReturn(true);
        $wrappedObject->getInstance()->willReturn($obj);
        $this->call('asort');
    }

    function it_delegates_throwing_class_not_found_exception(WrappedObject $wrappedObject, ExceptionFactory $exceptions)
    {
        $wrappedObject->isInstantiated()->willReturn(false);
        $wrappedObject->getClassName()->willReturn('Foo');

        $exceptions->classNotFound('Foo')
            ->willReturn(new \PhpSpec\Exception\Fracture\ClassNotFoundException(
                'Class "Foo" does not exist.',
                '"Foo"'
            ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Fracture\ClassNotFoundException')
            ->duringGetWrappedObject();
    }

    function it_delegates_throwing_method_not_found_exception(WrappedObject $wrappedObject, ExceptionFactory $exceptions)
    {
        $obj = new \ArrayObject();

        $wrappedObject->isInstantiated()->willReturn(true);
        $wrappedObject->getInstance()->willReturn($obj);
        $wrappedObject->getClassName()->willReturn('ArrayObject');

        $exceptions->methodNotFound('ArrayObject', 'foo', array())
            ->willReturn(new \PhpSpec\Exception\Fracture\MethodNotFoundException(
                'Method "foo" not found.',
                $obj,
                '"ArrayObject::foo"',
                array()
            ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Fracture\MethodNotFoundException')
            ->duringCall('foo');
    }

    function it_delegates_throwing_method_not_found_exception_for_constructor(WrappedObject $wrappedObject, ExceptionFactory $exceptions, \stdClass $argument)
    {
        $obj = new ExampleClass();

        $wrappedObject->isInstantiated()->willReturn(false);
        $wrappedObject->getInstance()->willReturn(null);
        $wrappedObject->getArguments()->willReturn(array($argument));
        $wrappedObject->getClassName()->willReturn('spec\PhpSpec\Wrapper\Subject\ExampleClass');
        $wrappedObject->getFactoryMethod()->willReturn(null);

        $exceptions->methodNotFound('spec\PhpSpec\Wrapper\Subject\ExampleClass', '__construct', array($argument))
            ->willReturn(new \PhpSpec\Exception\Fracture\MethodNotFoundException(
                    'Method "__construct" not found.',
                    $obj,
                    '"ExampleClass::__construct"',
                    array()
                ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Fracture\MethodNotFoundException')
            ->duringCall('__construct');
    }

    function it_delegates_throwing_named_constructor_not_found_exception(WrappedObject $wrappedObject, ExceptionFactory $exceptions)
    {
        $obj = new \ArrayObject();
        $arguments = array('firstname', 'lastname');

        $wrappedObject->isInstantiated()->willReturn(false);
        $wrappedObject->getInstance()->willReturn(null);
        $wrappedObject->getClassName()->willReturn('ArrayObject');
        $wrappedObject->getFactoryMethod()->willReturn('register');
        $wrappedObject->getArguments()->willReturn($arguments);

        $exceptions->namedConstructorNotFound('ArrayObject', 'register', $arguments)
            ->willReturn(new \PhpSpec\Exception\Fracture\NamedConstructorNotFoundException(
                'Named constructor "register" not found.',
                $obj,
                '"ArrayObject::register"',
                array()
            ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Fracture\NamedConstructorNotFoundException')
            ->duringCall('foo');
    }

    function it_delegates_throwing_method_not_visible_exception(WrappedObject $wrappedObject, ExceptionFactory $exceptions)
    {
        $obj = new ExampleClass();

        $wrappedObject->isInstantiated()->willReturn(true);
        $wrappedObject->getInstance()->willReturn($obj);
        $wrappedObject->getClassName()->willReturn('spec\PhpSpec\Wrapper\Subject\ExampleClass');

        $exceptions->methodNotVisible('spec\PhpSpec\Wrapper\Subject\ExampleClass', 'privateMethod', array())
            ->willReturn(new \PhpSpec\Exception\Fracture\MethodNotVisibleException(
                'Method "privateMethod" not visible.',
                $obj,
                '"ExampleClass::privateMethod"',
                array()
            ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Fracture\MethodNotVisibleException')
            ->duringCall('privateMethod');
    }

    function it_delegates_throwing_property_not_found_exception(WrappedObject $wrappedObject, ExceptionFactory $exceptions)
    {
        $obj = new ExampleClass();

        $wrappedObject->isInstantiated()->willReturn(true);
        $wrappedObject->getInstance()->willReturn($obj);

        $exceptions->propertyNotFound($obj, 'nonExistentProperty')
            ->willReturn(new \PhpSpec\Exception\Fracture\PropertyNotFoundException(
                'Property "nonExistentProperty" not found.',
                $obj,
                'nonExistentProperty'
            ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Fracture\PropertyNotFoundException')
            ->duringSet('nonExistentProperty', 'any value');
    }

    function it_delegates_throwing_calling_method_on_non_object_exception(ExceptionFactory $exceptions)
    {
        $exceptions->callingMethodOnNonObject('foo')
            ->willReturn(new \PhpSpec\Exception\Wrapper\SubjectException(
                'Call to a member function "foo()" on a non-object.'
            ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Wrapper\SubjectException')
            ->duringCall('foo');
    }

    function it_delegates_throwing_setting_property_on_non_object_exception(ExceptionFactory $exceptions)
    {
        $exceptions->settingPropertyOnNonObject('foo')
            ->willReturn(new \PhpSpec\Exception\Wrapper\SubjectException(
                'Setting property "foo" on a non-object.'
            ))
            ->shouldBeCalled();
        $this->shouldThrow('\PhpSpec\Exception\Wrapper\SubjectException')
            ->duringSet('foo');
    }

    function it_delegates_throwing_getting_property_on_non_object_exception(ExceptionFactory $exceptions)
    {
        $exceptions->gettingPropertyOnNonObject('foo')
            ->willReturn(new \PhpSpec\Exception\Wrapper\SubjectException(
                'Getting property "foo" on a non-object.'
            ))
            ->shouldBeCalled();

        $this->shouldThrow('\PhpSpec\Exception\Wrapper\SubjectException')
            ->duringGet('foo');
    }
}

class ExampleClass
{
    private function privateMethod()
    {
    }
}
