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

namespace PhpSpec\Runner;

use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use PhpSpec\Event;
use PhpSpec\Loader\Node\SpecificationNode;

class SpecificationRunner
{
    /**
     * @var \Symfony\Component\EventDispatcher\EventDispatcherInterface
     */
    private $dispatcher;
    /**
     * @var ExampleRunner
     */
    private $exampleRunner;

    /**
     * @param EventDispatcherInterface $dispatcher
     * @param ExampleRunner            $exampleRunner
     */
    public function __construct(EventDispatcherInterface $dispatcher, ExampleRunner $exampleRunner)
    {
        $this->dispatcher    = $dispatcher;
        $this->exampleRunner = $exampleRunner;
    }

    /**
     * @param  SpecificationNode $specification
     * @return int|mixed
     */
    public function run(SpecificationNode $specification)
    {
        $startTime = microtime(true);
        $this->dispatcher->dispatch('beforeSpecification',
            new Event\SpecificationEvent($specification)
        );

        $result = Event\ExampleEvent::PASSED;
        foreach ($specification->getExamples() as $example) {
            $result = max($result, $this->exampleRunner->run($example));
        }

        $this->dispatcher->dispatch('afterSpecification',
            new Event\SpecificationEvent($specification, microtime(true) - $startTime, $result)
        );

        return $result;
    }
}
