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

namespace PhpSpec\Formatter;

use PhpSpec\IO\IOInterface as IO;
use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\Listener\StatisticsCollector;
use PhpSpec\Event\SuiteEvent;
use PhpSpec\Event\SpecificationEvent;
use PhpSpec\Event\ExampleEvent;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

abstract class BasicFormatter implements EventSubscriberInterface
{
    /**
     * @var IO
     */
    private $io;

    /**
     * @var PresenterInterface
     */
    private $presenter;

    /**
     * @var StatisticsCollector
     */
    private $stats;

    public function __construct(PresenterInterface $presenter, IO $io, StatisticsCollector $stats)
    {
        $this->presenter = $presenter;
        $this->io = $io;
        $this->stats = $stats;
    }

    /**
     * @return array
     */
    public static function getSubscribedEvents()
    {
        $events = array(
            'beforeSuite', 'afterSuite',
            'beforeExample', 'afterExample',
            'beforeSpecification', 'afterSpecification'
        );

        return array_combine($events, $events);
    }

    /**
     * @return IO
     */
    protected function getIO()
    {
        return $this->io;
    }

    /**
     * @return PresenterInterface
     */
    protected function getPresenter()
    {
        return $this->presenter;
    }

    /**
     * @return StatisticsCollector
     */
    protected function getStatisticsCollector()
    {
        return $this->stats;
    }

    /**
     * @param SuiteEvent $event
     */
    public function beforeSuite(SuiteEvent $event)
    {
    }

    /**
     * @param SuiteEvent $event
     */
    public function afterSuite(SuiteEvent $event)
    {
    }

    /**
     * @param ExampleEvent $event
     */
    public function beforeExample(ExampleEvent $event)
    {
    }

    /**
     * @param ExampleEvent $event
     */
    public function afterExample(ExampleEvent $event)
    {
    }

    /**
     * @param SpecificationEvent $event
     */
    public function beforeSpecification(SpecificationEvent $event)
    {
    }

    /**
     * @param SpecificationEvent $event
     */
    public function afterSpecification(SpecificationEvent $event)
    {
    }
}
