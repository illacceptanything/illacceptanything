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

use PhpSpec\Wrapper\Unwrapper;

class UnwrapDecorator extends Decorator implements ExpectationInterface
{
    /**
     * @var \PhpSpec\Wrapper\Unwrapper
     */
    private $unwrapper;

    /**
     * @param ExpectationInterface $expectation
     * @param Unwrapper            $unwrapper
     */
    public function __construct(ExpectationInterface $expectation, Unwrapper $unwrapper)
    {
        $this->setExpectation($expectation);
        $this->unwrapper = $unwrapper;
    }

    /**
     * @param string $alias
     * @param mixed  $subject
     * @param array  $arguments
     *
     * @return mixed
     */
    public function match($alias, $subject, array $arguments = array())
    {
        $arguments = $this->unwrapper->unwrapAll($arguments);

        return $this->getExpectation()->match($alias, $subject, $arguments);
    }
}
