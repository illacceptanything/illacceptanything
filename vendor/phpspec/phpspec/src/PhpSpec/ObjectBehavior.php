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

namespace PhpSpec;

use PhpSpec\Matcher\MatchersProviderInterface;
use PhpSpec\Wrapper\WrapperInterface;
use PhpSpec\Wrapper\SubjectContainerInterface;
use PhpSpec\Wrapper\Subject;
use ArrayAccess;

/**
 * The object behaviour is the default base class for specification.
 *
 * Most specs will extend this class directly.
 *
 * Its responsibility is to proxy method calls to PhpSpec caller which will
 * wrap the results into PhpSpec subjects. This results will then be able to
 * be matched against expectations.
 *
 * @method void beConstructedWith()
 * @method void beConstructedThrough($factoryMethod, array $constructorArguments)
 * @method void beAnInstanceOf($class)
 * @method void shouldHaveType($type)
 * @method \PhpSpec\Wrapper\Subject\Expectation\DuringCall shouldThrow($exception = null)
 */
class ObjectBehavior implements ArrayAccess,
                                MatchersProviderInterface,
                                SubjectContainerInterface,
                                WrapperInterface,
                                SpecificationInterface
{
    /**
     * @var Subject
     */
    protected $object;

    /**
     * Override this method to provide your own inline matchers
     *
     * @link http://phpspec.net/cookbook/matchers.html Matchers cookbook
     * @return array a list of inline matchers
     */
    public function getMatchers()
    {
        return array();
    }

    /**
     * Used by { @link PhpSpec\Runner\Maintainer\SubjectMaintainer::prepare() }
     * to prepare the subject with all the needed collaborators for proxying
     * object behaviour
     *
     * @param Subject $subject
     */
    public function setSpecificationSubject(Subject $subject)
    {
        $this->object = $subject;
    }

    /**
     * Gets the unwrapped proxied object from PhpSpec subject
     *
     * @return object
     */
    public function getWrappedObject()
    {
        return $this->object->getWrappedObject();
    }

    /**
     * Checks if a key exists in case object implements ArrayAccess
     *
     * @param string|integer $key
     *
     * @return Subject
     */
    public function offsetExists($key)
    {
        return $this->object->offsetExists($key);
    }

    /**
     * Gets the value in a particular position in the ArrayAccess object
     *
     * @param string|integer $key
     *
     * @return Subject
     */
    public function offsetGet($key)
    {
        return $this->object->offsetGet($key);
    }

    /**
     * Sets the value in a particular position in the ArrayAccess object
     *
     * @param string|integer $key
     * @param mixed          $value
     */
    public function offsetSet($key, $value)
    {
        $this->object->offsetSet($key, $value);
    }

    /**
     * Unsets a position in the ArrayAccess object
     *
     * @param string|integer $key
     */
    public function offsetUnset($key)
    {
        return $this->object->offsetUnset($key);
    }

    /**
     * Proxies all calls to the PhpSpec subject
     *
     * @param string $method
     * @param array  $arguments
     *
     * @return mixed
     */
    public function __call($method, array $arguments = array())
    {
        return call_user_func_array(array($this->object, $method), $arguments);
    }

    /**
     * Proxies setting to the PhpSpec subject
     *
     * @param string $property
     * @param mixed  $value
     */
    public function __set($property, $value)
    {
        $this->object->$property = $value;
    }

    /**
     * Proxies getting to the PhpSpec subject
     *
     * @param string $property
     *
     * @return mixed
     */
    public function __get($property)
    {
        return $this->object->$property;
    }

    /**
     * Proxies functor calls to PhpSpec subject
     *
     * @return mixed
     */
    public function __invoke()
    {
        return call_user_func_array(array($this->object, '__invoke'), func_get_args());
    }
}
