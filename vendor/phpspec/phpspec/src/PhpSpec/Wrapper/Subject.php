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

namespace PhpSpec\Wrapper;

use PhpSpec\Wrapper\Subject\WrappedObject;
use PhpSpec\Wrapper\Subject\Caller;
use PhpSpec\Wrapper\Subject\SubjectWithArrayAccess;
use PhpSpec\Wrapper\Subject\ExpectationFactory;
use PhpSpec\Util\Instantiator;
use ArrayAccess;

class Subject implements ArrayAccess, WrapperInterface
{
    /**
     * @var mixed
     */
    private $subject;
    /**
     * @var Subject\WrappedObject
     */
    private $wrappedObject;
    /**
     * @var Subject\Caller
     */
    private $caller;
    /**
     * @var Subject\SubjectWithArrayAccess
     */
    private $arrayAccess;
    /**
     * @var Wrapper
     */
    private $wrapper;
    /**
     * @var Subject\ExpectationFactory
     */
    private $expectationFactory;

    /**
     * @param mixed                  $subject
     * @param Wrapper                $wrapper
     * @param WrappedObject          $wrappedObject
     * @param Caller                 $caller
     * @param SubjectWithArrayAccess $arrayAccess
     * @param ExpectationFactory     $expectationFactory
     */
    public function __construct($subject, Wrapper $wrapper, WrappedObject $wrappedObject, Caller $caller,
                                SubjectWithArrayAccess $arrayAccess, ExpectationFactory $expectationFactory)
    {
        $this->subject            = $subject;
        $this->wrapper            = $wrapper;
        $this->wrappedObject      = $wrappedObject;
        $this->caller             = $caller;
        $this->arrayAccess        = $arrayAccess;
        $this->expectationFactory = $expectationFactory;
    }

    /**
     * @param string $className
     * @param array  $arguments
     */
    public function beAnInstanceOf($className, array $arguments = array())
    {
        $this->wrappedObject->beAnInstanceOf($className, $arguments);
    }

    /**
     *
     */
    public function beConstructedWith()
    {
        $this->wrappedObject->beConstructedWith(func_get_args());
    }

    /**
     * @param array|string $factoryMethod
     * @param array        $arguments
     */
    public function beConstructedThrough($factoryMethod, array $arguments = array())
    {
        $this->wrappedObject->beConstructedThrough($factoryMethod, $arguments);
    }

    /**
     * @return mixed
     */
    public function getWrappedObject()
    {
        if ($this->subject) {
            return $this->subject;
        }

        return $this->subject = $this->caller->getWrappedObject();
    }

    /**
     * @param string $method
     * @param array  $arguments
     *
     * @return Subject
     */
    public function callOnWrappedObject($method, array $arguments = array())
    {
        return $this->caller->call($method, $arguments);
    }

    /**
     * @param string $property
     * @param mixed  $value
     *
     * @return mixed
     */
    public function setToWrappedObject($property, $value = null)
    {
        return $this->caller->set($property, $value);
    }

    /**
     * @param string $property
     *
     * @return string|Subject
     */
    public function getFromWrappedObject($property)
    {
        return $this->caller->get($property);
    }

    /**
     * @param string|integer $key
     *
     * @return Subject
     */
    public function offsetExists($key)
    {
        return $this->wrap($this->arrayAccess->offSetExists($key));
    }

    /**
     * @param string|integer $key
     *
     * @return Subject
     */
    public function offsetGet($key)
    {
        return $this->wrap($this->arrayAccess->offsetGet($key));
    }

    /**
     * @param string|integer $key
     * @param mixed          $value
     */
    public function offsetSet($key, $value)
    {
        $this->arrayAccess->offsetSet($key, $value);
    }

    /**
     * @param string|integer $key
     */
    public function offsetUnset($key)
    {
        $this->arrayAccess->offsetUnset($key);
    }

    /**
     * @param string $method
     * @param array  $arguments
     *
     * @return mixed|Subject
     */
    public function __call($method, array $arguments = array())
    {
        if (0 === strpos($method, 'should')) {
            return $this->callExpectation($method, $arguments);
        }

        return $this->caller->call($method, $arguments);
    }

    /**
     * @return Subject
     */
    public function __invoke()
    {
        return $this->caller->call('__invoke', func_get_args());
    }

    /**
     * @param string $property
     * @param mixed  $value
     *
     * @return mixed
     */
    public function __set($property, $value = null)
    {
        return $this->caller->set($property, $value);
    }

    /**
     * @param string $property
     *
     * @return string|Subject
     */
    public function __get($property)
    {
        return $this->caller->get($property);
    }

    /**
     * @param string $value
     *
     * @return Subject
     */
    private function wrap($value)
    {
        return $this->wrapper->wrap($value);
    }

    /**
     * @param string $method
     * @param array  $arguments
     *
     * @return mixed
     */
    private function callExpectation($method, array $arguments)
    {
        $subject = $this->makeSureWeHaveASubject();

        $expectation = $this->expectationFactory->create($method, $subject, $arguments);

        if (0 === strpos($method, 'shouldNot')) {
            return $expectation->match(lcfirst(substr($method, 9)), $this, $arguments, $this->wrappedObject);
        }

        return $expectation->match(lcfirst(substr($method, 6)), $this, $arguments, $this->wrappedObject);
    }

    /**
     * @return object
     */
    private function makeSureWeHaveASubject()
    {
        if (null === $this->subject && $this->wrappedObject->getClassname()) {
            $instantiator = new Instantiator();

            return $instantiator->instantiate($this->wrappedObject->getClassname());
        }

        return $this->subject;
    }
}
