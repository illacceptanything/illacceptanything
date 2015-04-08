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

class PassthruReRunner extends PhpExecutableReRunner
{
    /**
     * @return boolean
     */
    public function isSupported()
    {
        return (php_sapi_name() == 'cli')
            && $this->getExecutablePath()
            && function_exists('passthru');
    }

    public function reRunSuite()
    {
        $args = $_SERVER['argv'];
        $command = escapeshellcmd($this->getExecutablePath()).' '.join(' ', array_map('escapeshellarg', $args));
        passthru($command, $exitCode);
        exit($exitCode);
    }
}
