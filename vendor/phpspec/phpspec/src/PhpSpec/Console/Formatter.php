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

namespace PhpSpec\Console;

use Symfony\Component\Console\Formatter\OutputFormatter;
use Symfony\Component\Console\Formatter\OutputFormatterStyle;

/**
 * Class Formatter is responsible for configuring the console output formatting
 * style
 */
class Formatter extends OutputFormatter
{
    /**
     * @param boolean $decorated
     *
     * @param array $styles
     */
    public function __construct($decorated = false, array $styles = array())
    {
        parent::__construct($decorated, $styles);

        $this->setStyle('pending', new OutputFormatterStyle('yellow'));
        $this->setStyle('pending-bg', new OutputFormatterStyle('black', 'yellow', array('bold')));

        $this->setStyle('skipped', new OutputFormatterStyle('cyan'));
        $this->setStyle('skipped-bg', new OutputFormatterStyle('white', 'cyan', array('bold')));

        $this->setStyle('failed', new OutputFormatterStyle('red'));
        $this->setStyle('failed-bg', new OutputFormatterStyle('white', 'red', array('bold')));

        $this->setStyle('broken', new OutputFormatterStyle('magenta'));
        $this->setStyle('broken-bg', new OutputFormatterStyle('white', 'magenta', array('bold')));

        $this->setStyle('passed', new OutputFormatterStyle('green'));
        $this->setStyle('passed-bg', new OutputFormatterStyle('black', 'green', array('bold')));

        $this->setStyle('value', new OutputFormatterStyle('yellow'));
        $this->setStyle('lineno', new OutputFormatterStyle(null, 'black'));
        $this->setStyle('code', new OutputFormatterStyle('white'));
        $this->setStyle('hl', new OutputFormatterStyle('black', 'yellow', array('bold')));
        $this->setStyle('question', new OutputFormatterStyle('black', 'yellow', array('bold')));

        $this->setStyle('trace', new OutputFormatterStyle());
        $this->setStyle('trace-class', new OutputFormatterStyle('cyan'));
        $this->setStyle('trace-func', new OutputFormatterStyle('cyan'));
        $this->setStyle('trace-type', new OutputFormatterStyle());
        $this->setStyle('trace-args', new OutputFormatterStyle());

        $this->setStyle('diff-add', new OutputFormatterStyle('green'));
        $this->setStyle('diff-del', new OutputFormatterStyle('red'));
    }
}
