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

namespace PhpSpec\Formatter\Presenter;

interface PresenterInterface
{
    /**
     * @param mixed $value
     *
     * @return string
     */
    public function presentValue($value);

    /**
     * @param \Exception $exception
     * @param bool       $verbose
     *
     * @return string
     */
    public function presentException(\Exception $exception, $verbose = false);

    /**
     * @param string $string
     *
     * @return string
     */
    public function presentString($string);
}
