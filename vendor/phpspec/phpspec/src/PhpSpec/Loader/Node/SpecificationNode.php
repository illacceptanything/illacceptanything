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

namespace PhpSpec\Loader\Node;

use PhpSpec\Loader\Suite;
use PhpSpec\Locator\ResourceInterface;
use ReflectionClass;

class SpecificationNode implements \Countable
{
    /**
     * @var string
     */
    private $title;
    /**
     * @var \ReflectionClass
     */
    private $class;
    /**
     * @var \PhpSpec\Locator\ResourceInterface
     */
    private $resource;
    /**
     * @var Suite
     */
    private $suite;
    /**
     * @var ExampleNode[]
     */
    private $examples = array();

    /**
     * @param string            $title
     * @param ReflectionClass   $class
     * @param ResourceInterface $resource
     */
    public function __construct($title, ReflectionClass $class, ResourceInterface $resource)
    {
        $this->title    = $title;
        $this->class    = $class;
        $this->resource = $resource;
    }

    /**
     * @return string
     */
    public function getTitle()
    {
        return $this->title;
    }

    /**
     * @return ReflectionClass
     */
    public function getClassReflection()
    {
        return $this->class;
    }

    /**
     * @return ResourceInterface
     */
    public function getResource()
    {
        return $this->resource;
    }

    /**
     * @param ExampleNode $example
     */
    public function addExample(ExampleNode $example)
    {
        $this->examples[] = $example;
        $example->setSpecification($this);
    }

    /**
     * @return ExampleNode[]
     */
    public function getExamples()
    {
        return $this->examples;
    }

    /**
     * @param Suite $suite
     */
    public function setSuite(Suite $suite)
    {
        $this->suite = $suite;
    }

    /**
     * @return Suite|null
     */
    public function getSuite()
    {
        return $this->suite;
    }

    /**
     * @return int
     */
    public function count()
    {
        return count($this->examples);
    }
}
