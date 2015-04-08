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

namespace PhpSpec\Locator;

interface ResourceInterface
{
    /**
     * @return string
     */
    public function getName();

    /**
     * @return string
     */
    public function getSpecName();

    /**
     * @return string
     */
    public function getSrcFilename();

    /**
     * @return string
     */
    public function getSrcNamespace();

    /**
     * @return string
     */
    public function getSrcClassname();

    /**
     * @return string
     */
    public function getSpecFilename();

    /**
     * @return string
     */
    public function getSpecNamespace();

    /**
     * @return string
     */
    public function getSpecClassname();
}
