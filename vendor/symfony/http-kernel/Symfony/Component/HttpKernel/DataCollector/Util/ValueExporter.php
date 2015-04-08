<?php

/*
 * This file is part of the Symfony package.
 *
 * (c) Fabien Potencier <fabien@symfony.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace Symfony\Component\HttpKernel\DataCollector\Util;

/**
 * @author Bernhard Schussek <bschussek@gmail.com>
 */
class ValueExporter
{
    /**
     * Converts a PHP value to a string.
     *
     * @param mixed $value The PHP value
     * @param int   $depth only for internal usage
     * @param bool  $deep  only for internal usage
     *
     * @return string The string representation of the given value
     */
    public function exportValue($value, $depth = 1, $deep = false)
    {
        if (is_object($value)) {
            if ($value instanceof \DateTime || $value instanceof \DateTimeInterface) {
                return sprintf('Object(%s) - %s', get_class($value), $value->format(\DateTime::ISO8601));
            }

            return sprintf('Object(%s)', get_class($value));
        }

        if (is_array($value)) {
            if (empty($value)) {
                return '[]';
            }

            $indent = str_repeat('  ', $depth);

            $a = array();
            foreach ($value as $k => $v) {
                if (is_array($v)) {
                    $deep = true;
                }
                $a[] = sprintf('%s => %s', $k, $this->exportValue($v, $depth + 1, $deep));
            }

            if ($deep) {
                return sprintf("[\n%s%s\n%s]", $indent, implode(sprintf(", \n%s", $indent), $a), str_repeat('  ', $depth - 1));
            }

            return sprintf("[%s]", implode(', ', $a));
        }

        if (is_resource($value)) {
            return sprintf('Resource(%s#%d)', get_resource_type($value), $value);
        }

        if (null === $value) {
            return 'null';
        }

        if (false === $value) {
            return 'false';
        }

        if (true === $value) {
            return 'true';
        }

        return (string) $value;
    }
}
