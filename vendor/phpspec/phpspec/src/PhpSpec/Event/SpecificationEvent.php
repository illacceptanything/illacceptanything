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

namespace PhpSpec\Event;

use Symfony\Component\EventDispatcher\Event;
use PhpSpec\Loader\Node\SpecificationNode;

/**
 * Class SpecificationEvent holds information about the specification event
 */
class SpecificationEvent extends Event implements EventInterface
{
    /**
     * @var \PhpSpec\Loader\Node\SpecificationNode
     */
    private $specification;

    /**
     * @var float
     */
    private $time;

    /**
     * @var integer
     */
    private $result;

    /**
     * @param SpecificationNode $specification
     * @param float             $time
     * @param integer           $result
     */
    public function __construct(SpecificationNode $specification, $time = null, $result = null)
    {
        $this->specification = $specification;
        $this->time          = $time;
        $this->result        = $result;
    }

    /**
     * @return SpecificationNode
     */
    public function getSpecification()
    {
        return $this->specification;
    }

    /**
     * @return string
     */
    public function getTitle()
    {
        return $this->specification->getTitle();
    }

    /**
     * @return \PhpSpec\Loader\Suite
     */
    public function getSuite()
    {
        return $this->specification->getSuite();
    }

    /**
     * @return float
     */
    public function getTime()
    {
        return $this->time;
    }

    /**
     * @return integer
     */
    public function getResult()
    {
        return $this->result;
    }
}
