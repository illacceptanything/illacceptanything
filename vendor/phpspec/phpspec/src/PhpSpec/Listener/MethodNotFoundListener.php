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

namespace PhpSpec\Listener;

use Symfony\Component\EventDispatcher\EventSubscriberInterface;
use PhpSpec\Console\IO;
use PhpSpec\Locator\ResourceManagerInterface;
use PhpSpec\CodeGenerator\GeneratorManager;
use PhpSpec\Event\ExampleEvent;
use PhpSpec\Event\SuiteEvent;
use PhpSpec\Exception\Fracture\MethodNotFoundException;

class MethodNotFoundListener implements EventSubscriberInterface
{
    private $io;
    private $resources;
    private $generator;
    private $methods = array();

    public function __construct(IO $io, ResourceManagerInterface $resources, GeneratorManager $generator)
    {
        $this->io        = $io;
        $this->resources = $resources;
        $this->generator = $generator;
    }

    public static function getSubscribedEvents()
    {
        return array(
            'afterExample' => array('afterExample', 10),
            'afterSuite'   => array('afterSuite', -10),
        );
    }

    public function afterExample(ExampleEvent $event)
    {
        if (null === $exception = $event->getException()) {
            return;
        }

        if (!$exception instanceof MethodNotFoundException) {
            return;
        }

        $this->methods[get_class($exception->getSubject()).'::'.$exception->getMethodName()] = $exception->getArguments();
    }

    public function afterSuite(SuiteEvent $event)
    {
        if (!$this->io->isCodeGenerationEnabled()) {
            return;
        }

        foreach ($this->methods as $call => $arguments) {
            list($classname, $method) = explode('::', $call);
            $message = sprintf('Do you want me to create `%s()` for you?', $call);

            try {
                $resource = $this->resources->createResource($classname);
            } catch (\RuntimeException $e) {
                continue;
            }

            if ($this->io->askConfirmation($message)) {
                $this->generator->generate($resource, 'method', array(
                    'name'      => $method,
                    'arguments' => $arguments
                ));
                $event->markAsWorthRerunning();
            }
        }
    }
}
