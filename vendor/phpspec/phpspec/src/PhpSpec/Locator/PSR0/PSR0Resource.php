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

namespace PhpSpec\Locator\PSR0;

use PhpSpec\Locator\ResourceInterface;

class PSR0Resource implements ResourceInterface
{
    /**
     * @var array
     */
    private $parts;
    /**
     * @var PSR0Locator
     */
    private $locator;

    /**
     * @param array       $parts
     * @param PSR0Locator $locator
     */
    public function __construct(array $parts, PSR0Locator $locator)
    {
        $this->parts   = $parts;
        $this->locator = $locator;
    }

    /**
     * @return mixed
     */
    public function getName()
    {
        return end($this->parts);
    }

    /**
     * @return string
     */
    public function getSpecName()
    {
        return $this->getName().'Spec';
    }

    /**
     * @return string
     */
    public function getSrcFilename()
    {
        $nsParts   = $this->parts;
        $classname = array_pop($nsParts);
        $parts     = array_merge($nsParts, explode('_', $classname));

        return $this->locator->getFullSrcPath().implode(DIRECTORY_SEPARATOR, $parts).'.php';
    }

    /**
     * @return string
     */
    public function getSrcNamespace()
    {
        $nsParts = $this->parts;
        array_pop($nsParts);

        return rtrim($this->locator->getSrcNamespace().implode('\\', $nsParts), '\\');
    }

    /**
     * @return string
     */
    public function getSrcClassname()
    {
        return $this->locator->getSrcNamespace().implode('\\', $this->parts);
    }

    /**
     * @return string
     */
    public function getSpecFilename()
    {
        $nsParts   = $this->parts;
        $classname = array_pop($nsParts);
        $parts     = array_merge($nsParts, explode('_', $classname));

        return $this->locator->getFullSpecPath().
            implode(DIRECTORY_SEPARATOR, $parts).'Spec.php';
    }

    /**
     * @return string
     */
    public function getSpecNamespace()
    {
        $nsParts = $this->parts;
        array_pop($nsParts);

        return rtrim($this->locator->getSpecNamespace().implode('\\', $nsParts), '\\');
    }

    /**
     * @return string
     */
    public function getSpecClassname()
    {
        return $this->locator->getSpecNamespace().implode('\\', $this->parts).'Spec';
    }
}
