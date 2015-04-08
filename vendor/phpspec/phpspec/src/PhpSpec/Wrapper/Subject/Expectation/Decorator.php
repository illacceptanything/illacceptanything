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

abstract class Decorator implements ExpectationInterface
{
    /**
     * @var ExpectationInterface
     */
    private $expectation;

    /**
     * @param ExpectationInterface $expectation
     */
    public function __construct(ExpectationInterface $expectation)
    {
        $this->expectation = $expectation;
    }

    /**
     * @return ExpectationInterface
     */
    public function getExpectation()
    {
        return $this->expectation;
    }

    /**
     * @param ExpectationInterface $expectation
     */
    protected function setExpectation(ExpectationInterface $expectation)
    {
        $this->expectation = $expectation;
    }

    /**
     * @return ExpectationInterface
     */
    public function getNestedExpectation()
    {
        $expectation = $this->getExpectation();
        while ($expectation instanceof Decorator) {
            $expectation = $expectation->getExpectation();
        }

        return $expectation;
    }
}
