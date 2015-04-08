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

namespace PhpSpec\Factory;

/**
 * Class ReflectionFactory is a simple factory wrapper to create reflection
 * classes
 */
class ReflectionFactory
{
    /**
     * @param $class
     * @return \ReflectionClass
     */
    public function create($class)
    {
        return new \ReflectionClass($class);
    }
}
