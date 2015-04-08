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

use PhpSpec\Event\SuiteEvent;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;
use PhpSpec\Event\ExampleEvent;
use PhpSpec\Event\SpecificationEvent;

class StatisticsCollector implements EventSubscriberInterface
{
    private $globalResult    = 0;
    private $totalSpecs      = 0;
    private $totalSpecsCount = 0;

    private $passedEvents  = array();
    private $pendingEvents = array();
    private $skippedEvents = array();
    private $failedEvents  = array();
    private $brokenEvents  = array();

    public static function getSubscribedEvents()
    {
        return array(
            'afterSpecification' => array('afterSpecification', 10),
            'afterExample'       => array('afterExample', 10),
            'beforeSuite'       => array('beforeSuite', 10),

        );
    }

    public function afterSpecification(SpecificationEvent $event)
    {
        $this->totalSpecs++;
    }

    public function afterExample(ExampleEvent $event)
    {
        $this->globalResult = max($this->globalResult, $event->getResult());

        switch ($event->getResult()) {
            case ExampleEvent::PASSED:
                $this->passedEvents[] = $event;
                break;
            case ExampleEvent::PENDING:
                $this->pendingEvents[] = $event;
                break;
            case ExampleEvent::SKIPPED:
                $this->skippedEvents[] = $event;
                break;
            case ExampleEvent::FAILED:
                $this->failedEvents[] = $event;
                break;
            case ExampleEvent::BROKEN:
                $this->brokenEvents[] = $event;
                break;
        }
    }

    public function beforeSuite(SuiteEvent $suiteEvent)
    {
        $this->totalSpecsCount = count($suiteEvent->getSuite()->getSpecifications());
    }

    public function getGlobalResult()
    {
        return $this->globalResult;
    }

    public function getAllEvents()
    {
        return array_merge(
            $this->passedEvents,
            $this->pendingEvents,
            $this->skippedEvents,
            $this->failedEvents,
            $this->brokenEvents
        );
    }

    public function getPassedEvents()
    {
        return $this->passedEvents;
    }

    public function getPendingEvents()
    {
        return $this->pendingEvents;
    }

    public function getSkippedEvents()
    {
        return $this->skippedEvents;
    }

    public function getFailedEvents()
    {
        return $this->failedEvents;
    }

    public function getBrokenEvents()
    {
        return $this->brokenEvents;
    }

    public function getCountsHash()
    {
        return array(
            'passed'  => count($this->getPassedEvents()),
            'pending' => count($this->getPendingEvents()),
            'skipped' => count($this->getSkippedEvents()),
            'failed'  => count($this->getFailedEvents()),
            'broken'  => count($this->getBrokenEvents()),
        );
    }

    public function getTotalSpecs()
    {
        return $this->totalSpecs;
    }

    public function getEventsCount()
    {
        return count($this->getAllEvents());
    }

    public function getTotalSpecsCount()
    {
        return $this->totalSpecsCount;
    }
}
