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

namespace PhpSpec\Exception\Example;

/**
 * Class NotEqualException holds information about the non-equal failure exception
 */
class NotEqualException extends FailureException
{
    /**
     * @var mixed
     */
    private $expected;

    /**
     * @var mixed
     */
    private $actual;

    /**
     * @param string $message
     * @param mixed  $expected
     * @param mixed  $actual
     */
    public function __construct($message, $expected, $actual)
    {
        parent::__construct($message);

        $this->expected = $expected;
        $this->actual   = $actual;
    }

    /**
     * @return mixed
     */
    public function getExpected()
    {
        return $this->expected;
    }

    /**
     * @return mixed
     */
    public function getActual()
    {
        return $this->actual;
    }

    /**
     * @return string
     */
    public function __toString()
    {
        return var_export(array($this->expected, $this->actual), true);
    }
}
