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

namespace PhpSpec\Formatter\Presenter\Differ;

class ArrayEngine extends StringEngine
{
    public function supports($expected, $actual)
    {
        return is_array($expected) && is_array($actual);
    }

    public function compare($expected, $actual)
    {
        $expectedString = $this->convertArrayToString($expected);
        $actualString   = $this->convertArrayToString($actual);

        return parent::compare($expectedString, $actualString);
    }

    private function convertArrayToString(array $a, $pad = 2)
    {
        $str = str_pad('', $pad, ' ').'[';
        foreach ($a as $key => $val) {
            switch ($type = strtolower(gettype($val))) {
                case 'array':
                    $line = sprintf('%s => %s,',
                        $key,
                        ltrim($this->convertArrayToString($val, $pad+2))
                    );
                    break;
                case 'null':
                    $line = sprintf('%s => null,', $key);
                    break;
                case 'boolean':
                    $line = sprintf('%s => %s,', $key, $val ? 'true' : 'false');
                    break;
                case 'object':
                    $line = sprintf('%s => %s#%s,',
                        $key,
                        get_class($val),
                        spl_object_hash($val)
                    );
                    break;
                case 'string':
                    if (25 > strlen($val) && false === strpos($val, PHP_EOL)) {
                        $val = sprintf('"%s"', $val);
                    }

                    $lines = explode(PHP_EOL, $val);
                    $val = sprintf('"%s..."', substr($lines[0], 0, 25));

                    $line = sprintf('%s => %s,', $key, $val);
                    break;
                case 'integer':
                case 'double':
                    $line = sprintf('%s => %s,', $key, $val);
                    break;
                default:
                    $line = sprintf('%s => %s:%s,', $key, $type, $val);
            }
            $str .= PHP_EOL.str_pad('', $pad+2, ' ').$line;
        }
        $str .= PHP_EOL.str_pad('', $pad, ' ').']';

        return $str;
    }
}
