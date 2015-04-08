<?php

/*
 * This file is part of the Symfony package.
 *
 * (c) Fabien Potencier <fabien@symfony.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace Symfony\Component\HttpKernel\Exception;

/**
 * Fatal Error Exception.
 *
 * @author Fabien Potencier <fabien@symfony.com>
 * @author Konstanton Myakshin <koc-dp@yandex.ru>
 * @author Nicolas Grekas <p@tchwork.com>
 *
 * @deprecated Deprecated in 2.3, to be removed in 3.0. Use the same class from the Debug component instead.
 */
class FatalErrorException extends \ErrorException
{
}

namespace Symfony\Component\Debug\Exception;

use Symfony\Component\HttpKernel\Exception\FatalErrorException as LegacyFatalErrorException;

/**
 * Fatal Error Exception.
 *
 * @author Konstanton Myakshin <koc-dp@yandex.ru>
 */
class FatalErrorException extends LegacyFatalErrorException
{
    public function __construct($message, $code, $severity, $filename, $lineno, $traceOffset = null, $traceArgs = true)
    {
        parent::__construct($message, $code, $severity, $filename, $lineno);

        if (null !== $traceOffset) {
            if (function_exists('xdebug_get_function_stack')) {
                $trace = xdebug_get_function_stack();
                if (0 < $traceOffset) {
                    array_splice($trace, -$traceOffset);
                }

                foreach ($trace as &$frame) {
                    if (!isset($frame['type'])) {
                        //  XDebug pre 2.1.1 doesn't currently set the call type key http://bugs.xdebug.org/view.php?id=695
                        if (isset($frame['class'])) {
                            $frame['type'] = '::';
                        }
                    } elseif ('dynamic' === $frame['type']) {
                        $frame['type'] = '->';
                    } elseif ('static' === $frame['type']) {
                        $frame['type'] = '::';
                    }

                    // XDebug also has a different name for the parameters array
                    if (!$traceArgs) {
                        unset($frame['params'], $frame['args']);
                    } elseif (isset($frame['params']) && !isset($frame['args'])) {
                        $frame['args'] = $frame['params'];
                        unset($frame['params']);
                    }
                }

                unset($frame);
                $trace = array_reverse($trace);
            } else {
                $trace = array();
            }

            $this->setTrace($trace);
        }
    }

    protected function setTrace($trace)
    {
        $traceReflector = new \ReflectionProperty('Exception', 'trace');
        $traceReflector->setAccessible(true);
        $traceReflector->setValue($this, $trace);
    }
}
