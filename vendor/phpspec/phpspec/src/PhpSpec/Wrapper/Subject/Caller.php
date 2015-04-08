<?php

/*
 * This file is part of PhpSpec, A php toolset to drive emergent
 * design by specification.
 *
 * (c) Marcello Duarte <marcello.duarte@gmail.com>
 * (c) Konstantin Kudryashov <ever.zet@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace PhpSpec\Wrapper\Subject;

use PhpSpec\Exception\ExceptionFactory;
use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\Wrapper\Subject;
use PhpSpec\Wrapper\Wrapper;
use PhpSpec\Wrapper\Unwrapper;
use PhpSpec\Event\MethodCallEvent;
use Symfony\Component\EventDispatcher\EventDispatcherInterface as Dispatcher;
use ReflectionClass;
use ReflectionMethod;
use ReflectionProperty;
use ReflectionException;

class Caller
{
    /**
     * @var WrappedObject
     */
    private $wrappedObject;
    /**
     * @var \PhpSpec\Loader\Node\ExampleNode
     */
    private $example;
    /**
     * @var \Symfony\Component\EventDispatcher\EventDispatcherInterface
     */
    private $dispatcher;
    /**
     * @var \PhpSpec\Wrapper\Wrapper
     */
    private $wrapper;
    /**
     * @var \PhpSpec\Exception\ExceptionFactory
     */
    private $exceptionFactory;

    /**
     * @param WrappedObject    $wrappedObject
     * @param ExampleNode      $example
     * @param Dispatcher       $dispatcher
     * @param ExceptionFactory $exceptions
     * @param Wrapper          $wrapper
     */
    public function __construct(WrappedObject $wrappedObject, ExampleNode $example, Dispatcher $dispatcher,
                                ExceptionFactory $exceptions, Wrapper $wrapper)
    {
        $this->wrappedObject    = $wrappedObject;
        $this->example          = $example;
        $this->dispatcher       = $dispatcher;
        $this->wrapper          = $wrapper;
        $this->exceptionFactory = $exceptions;
    }

    /**
     * @param string $method
     * @param array  $arguments
     *
     * @return Subject
     *
     * @throws \PhpSpec\Exception\Fracture\MethodNotFoundException
     * @throws \PhpSpec\Exception\Fracture\MethodNotVisibleException
     * @throws \PhpSpec\Exception\Wrapper\SubjectException
     */
    public function call($method, array $arguments = array())
    {
        if (null === $this->getWrappedObject()) {
            throw $this->callingMethodOnNonObject($method);
        }

        $subject   = $this->wrappedObject->getInstance();
        $unwrapper = new Unwrapper();
        $arguments = $unwrapper->unwrapAll($arguments);

        if ($this->isObjectMethodAccessible($method)) {
            return $this->invokeAndWrapMethodResult($subject, $method, $arguments);
        }

        throw $this->methodNotFound($method, $arguments);
    }

    /**
     * @param string $property
     * @param mixed  $value
     *
     * @return mixed
     *
     * @throws \PhpSpec\Exception\Wrapper\SubjectException
     * @throws \PhpSpec\Exception\Fracture\PropertyNotFoundException
     */
    public function set($property, $value = null)
    {
        if (null === $this->getWrappedObject()) {
            throw $this->settingPropertyOnNonObject($property);
        }

        $unwrapper = new Unwrapper();
        $value = $unwrapper->unwrapOne($value);

        if ($this->isObjectPropertyAccessible($property, true)) {
            return $this->getWrappedObject()->$property = $value;
        }

        throw $this->propertyNotFound($property);
    }

    /**
     * @param string $property
     *
     * @return Subject|string
     *
     * @throws \PhpSpec\Exception\Fracture\PropertyNotFoundException
     * @throws \PhpSpec\Exception\Wrapper\SubjectException
     */
    public function get($property)
    {
        if ($this->lookingForConstants($property) && $this->constantDefined($property)) {
            return constant($this->wrappedObject->getClassName().'::'.$property);
        }

        if (null === $this->getWrappedObject()) {
            throw $this->accessingPropertyOnNonObject($property);
        }

        if ($this->isObjectPropertyAccessible($property)) {
            return $this->wrap($this->getWrappedObject()->$property);
        }

        throw $this->propertyNotFound($property);
    }

    /**
     * @return object
     *
     * @throws \PhpSpec\Exception\Fracture\ClassNotFoundException
     */
    public function getWrappedObject()
    {
        if ($this->wrappedObject->isInstantiated()) {
            return $this->wrappedObject->getInstance();
        }

        if (null === $this->wrappedObject->getClassName() || !is_string($this->wrappedObject->getClassName())) {
            return $this->wrappedObject->getInstance();
        }

        if (!class_exists($this->wrappedObject->getClassName())) {
            throw $this->classNotFound();
        }

        if (is_object($this->wrappedObject->getInstance())) {
            $this->wrappedObject->setInstantiated(true);
            $instance = $this->wrappedObject->getInstance();
        } else {
            $instance = $this->instantiateWrappedObject();
            $this->wrappedObject->setInstance($instance);
            $this->wrappedObject->setInstantiated(true);
        }

        return $instance;
    }

    /**
     * @param string $property
     * @param bool   $withValue
     *
     * @return bool
     */
    private function isObjectPropertyAccessible($property, $withValue = false)
    {
        if (!is_object($this->getWrappedObject())) {
            return false;
        }

        if (method_exists($this->getWrappedObject(), $withValue ? '__set' : '__get')) {
            return true;
        }

        if (!property_exists($this->getWrappedObject(), $property)) {
            return false;
        }

        $propertyReflection = new ReflectionProperty($this->getWrappedObject(), $property);

        return $propertyReflection->isPublic();
    }

    /**
     * @param string $method
     *
     * @return bool
     */
    private function isObjectMethodAccessible($method)
    {
        if (!is_object($this->getWrappedObject())) {
            return false;
        }

        if (method_exists($this->getWrappedObject(), '__call')) {
            return true;
        }

        if (!method_exists($this->getWrappedObject(), $method)) {
            return false;
        }

        $methodReflection = new ReflectionMethod($this->getWrappedObject(), $method);

        return $methodReflection->isPublic();
    }

    /**
     * @return object
     */
    private function instantiateWrappedObject()
    {
        if ($this->wrappedObject->getFactoryMethod()) {
            return $this->newInstanceWithFactoryMethod();
        }

        $reflection = new ReflectionClass($this->wrappedObject->getClassName());

        if (count($this->wrappedObject->getArguments())) {
            return $this->newInstanceWithArguments($reflection);
        }

        return $reflection->newInstance();
    }

    /**
     * @param object $subject
     * @param string $method
     * @param array  $arguments
     *
     * @return Subject
     */
    private function invokeAndWrapMethodResult($subject, $method, array $arguments = array())
    {
        $this->dispatcher->dispatch('beforeMethodCall',
            new MethodCallEvent($this->example, $subject, $method, $arguments)
        );

        $returnValue = call_user_func_array(array($subject, $method), $arguments);

        $this->dispatcher->dispatch('afterMethodCall',
            new MethodCallEvent($this->example, $subject, $method, $arguments)
        );

        return $this->wrap($returnValue);
    }

    /**
     * @param mixed $value
     *
     * @return Subject
     */
    private function wrap($value)
    {
        return $this->wrapper->wrap($value);
    }

    /**
     * @param ReflectionClass $reflection
     *
     * @return object
     *
     * @throws \PhpSpec\Exception\Fracture\MethodNotFoundException
     * @throws \PhpSpec\Exception\Fracture\MethodNotVisibleException
     * @throws \Exception
     * @throws \ReflectionException
     */
    private function newInstanceWithArguments(ReflectionClass $reflection)
    {
        try {
            return $reflection->newInstanceArgs($this->wrappedObject->getArguments());
        } catch (ReflectionException $e) {
            if ($this->detectMissingConstructorMessage($e)) {
                throw $this->methodNotFound(
                    '__construct', $this->wrappedObject->getArguments()
                );
            }
            throw $e;
        }
    }

    /**
     * @return mixed
     * @throws \PhpSpec\Exception\Fracture\MethodNotFoundException
     */
    private function newInstanceWithFactoryMethod()
    {
        $method = $this->wrappedObject->getFactoryMethod();

        if (!is_array($method)) {
            $className = $this->wrappedObject->getClassName();

            if (!method_exists($className, $method)) {
                throw $this->namedConstructorNotFound(
                    $method, $this->wrappedObject->getArguments()
                );
            }
        }

        return call_user_func_array($method, $this->wrappedObject->getArguments());
    }

    /**
     * @param ReflectionException $exception
     *
     * @return bool
     */
    private function detectMissingConstructorMessage(ReflectionException $exception)
    {
        return strpos(
            $exception->getMessage(), 'does not have a constructor'
        ) !== 0;
    }

    /**
     * @return \PhpSpec\Exception\Fracture\ClassNotFoundException
     */
    private function classNotFound()
    {
        return $this->exceptionFactory->classNotFound($this->wrappedObject->getClassName());
    }

    /**
     * @param string $method
     * @param array  $arguments
     *
     * @return \PhpSpec\Exception\Fracture\MethodNotFoundException|\PhpSpec\Exception\Fracture\MethodNotVisibleException
     */
    private function namedConstructorNotFound($method, array $arguments = array())
    {
        $className = $this->wrappedObject->getClassName();

        return $this->exceptionFactory->namedConstructorNotFound($className, $method, $arguments);
    }

    /**
     * @param $method
     * @param  array                                                                                                     $arguments
     * @return \PhpSpec\Exception\Fracture\MethodNotFoundException|\PhpSpec\Exception\Fracture\MethodNotVisibleException
     */
    private function methodNotFound($method, array $arguments = array())
    {
        $className = $this->wrappedObject->getClassName();

        if (!method_exists($className, $method)) {
            return $this->exceptionFactory->methodNotFound($className, $method, $arguments);
        }

        return $this->exceptionFactory->methodNotVisible($className, $method, $arguments);
    }

    /**
     * @param string $property
     *
     * @return \PhpSpec\Exception\Fracture\PropertyNotFoundException
     */
    private function propertyNotFound($property)
    {
        return $this->exceptionFactory->propertyNotFound($this->getWrappedObject(), $property);
    }

    /**
     * @param string $method
     *
     * @return \PhpSpec\Exception\Wrapper\SubjectException
     */
    private function callingMethodOnNonObject($method)
    {
        return $this->exceptionFactory->callingMethodOnNonObject($method);
    }

    /**
     * @param string $property
     *
     * @return \PhpSpec\Exception\Wrapper\SubjectException
     */
    private function settingPropertyOnNonObject($property)
    {
        return $this->exceptionFactory->settingPropertyOnNonObject($property);
    }

    /**
     * @param string $property
     *
     * @return \PhpSpec\Exception\Wrapper\SubjectException
     */
    private function accessingPropertyOnNonObject($property)
    {
        return $this->exceptionFactory->gettingPropertyOnNonObject($property);
    }

    /**
     * @param string $property
     *
     * @return bool
     */
    private function lookingForConstants($property)
    {
        return null !== $this->wrappedObject->getClassName() &&
            $property === strtoupper($property);
    }

    /**
     * @param string $property
     *
     * @return bool
     */
    public function constantDefined($property)
    {
        return defined($this->wrappedObject->getClassName().'::'.$property);
    }
}
