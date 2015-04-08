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

use PhpSpec\Console\IO;
use PhpSpec\Process\ReRunner;

class OptionalReRunner implements ReRunner
{
    /**
     * @var \PhpSpec\Console\IO
     */
    private $io;
    /**
     * @var \PhpSpec\Process\ReRunner
     */
    private $decoratedRerunner;

    /**
     * @param IO $io
     */
    public function __construct(ReRunner $decoratedRerunner, IO $io)
    {
        $this->io = $io;
        $this->decoratedRerunner = $decoratedRerunner;
    }

    public function reRunSuite()
    {
        if ($this->io->isRerunEnabled()) {
            $this->decoratedRerunner->reRunSuite();
        }
    }
}
