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

namespace PhpSpec\Formatter\Html;

use PhpSpec\Formatter\Presenter\StringPresenter;
use Exception;
use PhpSpec\Exception\Exception as PhpSpecException;

class HtmlPresenter extends StringPresenter
{
    /**
     * @param Exception $exception
     * @param bool      $verbose
     *
     * @return string
     */
    public function presentException(Exception $exception, $verbose = false)
    {
        if ($exception instanceof PhpSpecException) {
            list($file, $line) = $this->getExceptionExamplePosition($exception);

            return $this->presentFileCode($file, $line);
        }
    }

    /**
     * @param string  $file
     * @param integer $lineno
     * @param integer $context
     *
     * @return string
     */
    protected function presentFileCode($file, $lineno, $context = 6)
    {
        $lines  = explode("\n", file_get_contents($file));
        $offset = max(0, $lineno - ceil($context / 2));
        $lines  = array_slice($lines, $offset, $context);

        $text = "\n";
        foreach ($lines as $line) {
            $offset++;

            if ($offset == $lineno) {
                $cssClass = "offending";
            } else {
                $cssClass = "normal";
            }
            $text .= '<span class="linenum">'.$offset.'</span><span class="'.
                     $cssClass.'">'.$line.'</span>';

            $text .= "\n";
        }

        return $text;
    }
}
