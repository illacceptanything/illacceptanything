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

namespace PhpSpec\Loader;

class Suite implements \Countable
{
    /**
     * @var array
     */
    private $specs = array();

    /**
     * @param Node\SpecificationNode $spec
     */
    public function addSpecification(Node\SpecificationNode $spec)
    {
        $this->specs[] = $spec;
        $spec->setSuite($this);
    }

    /**
     * @return Node\SpecificationNode[]
     */
    public function getSpecifications()
    {
        return $this->specs;
    }

    /**
     * @return number
     */
    public function count()
    {
        return array_sum(array_map('count', $this->specs));
    }
}
