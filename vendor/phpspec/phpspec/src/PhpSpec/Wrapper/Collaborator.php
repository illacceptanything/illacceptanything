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

use Prophecy\Prophecy\ObjectProphecy;

class Collaborator implements WrapperInterface
{
    /**
     * @var \Prophecy\Prophecy\ObjectProphecy
     */
    private $prophecy;

    /**
     * @param ObjectProphecy $prophecy
     */
    public function __construct(ObjectProphecy $prophecy)
    {
        $this->prophecy  = $prophecy;
    }

    /**
     * @param string $classOrInterface
     */
    public function beADoubleOf($classOrInterface)
    {
        if (interface_exists($classOrInterface)) {
            $this->prophecy->willImplement($classOrInterface);
        } else {
            $this->prophecy->willExtend($classOrInterface);
        }
    }

    /**
     * @param array $arguments
     */
    public function beConstructedWith(array $arguments = null)
    {
        $this->prophecy->willBeConstructedWith($arguments);
    }

    /**
     * @param string $interface
     */
    public function implement($interface)
    {
        $this->prophecy->willImplement($interface);
    }

    /**
     * @param string $method
     * @param array  $arguments
     *
     * @return mixed
     */
    public function __call($method, array $arguments)
    {
        return call_user_func_array(array($this->prophecy, '__call'), array($method, $arguments));
    }

    /**
     * @param string $parameter
     * @param mixed  $value
     */
    public function __set($parameter, $value)
    {
        $this->prophecy->$parameter = $value;
    }

    /**
     * @param string $parameter
     *
     * @return mixed
     */
    public function __get($parameter)
    {
        return $this->prophecy->$parameter;
    }

    /**
     * @return object
     */
    public function getWrappedObject()
    {
        return $this->prophecy->reveal();
    }
}
