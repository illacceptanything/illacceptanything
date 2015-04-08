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

namespace PhpSpec\Process\ReRunner;

use PhpSpec\Process\ReRunner;
use Symfony\Component\Process\PhpExecutableFinder;

abstract class PhpExecutableReRunner implements PlatformSpecificReRunner
{
    /**
     * @var PhpExecutableFinder
     */
    private $executableFinder;

    /**
     * @var null|false|string
     */
    private $executablePath;

    /**
     * @param PhpExecutableFinder $executableFinder
     */
    public function __construct(PhpExecutableFinder $executableFinder)
    {
        $this->executableFinder = $executableFinder;
    }

    /**
     * @return false|string
     */
    protected function getExecutablePath()
    {
        if (null === $this->executablePath) {
            $this->executablePath = $this->executableFinder->find();
        }

        return $this->executablePath;
    }
}
