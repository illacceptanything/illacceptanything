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

namespace PhpSpec\Wrapper\Subject\Expectation;

class NegativeThrow extends DuringCall implements ThrowExpectation
{
    /**
     * @param object $object
     * @param string $method
     * @param array  $arguments
     *
     * @return mixed
     */
    protected function runDuring($object, $method, array $arguments = array())
    {
        return $this->getMatcher()->negativeMatch('throw', $object, $this->getArguments())
            ->during($method, $arguments);
    }
}
