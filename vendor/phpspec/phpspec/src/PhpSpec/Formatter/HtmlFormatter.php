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

use PhpSpec\Event\ExampleEvent;
use PhpSpec\Event\SuiteEvent;
use PhpSpec\Event\SpecificationEvent;
use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\IO\IOInterface as IO;
use PhpSpec\Listener\StatisticsCollector;

class HtmlFormatter extends BasicFormatter
{
    /**
     * @var Html\ReportItemFactory
     */
    private $reportItemFactory;

    /**
     * @var int
     */
    private $index = 1;

    public function __construct(Html\ReportItemFactory $reportItemFactory, PresenterInterface $presenter, IO $io, StatisticsCollector $stats)
    {
        $this->reportItemFactory = $reportItemFactory;

        parent::__construct($presenter, $io, $stats);
    }

    /**
     * @param SuiteEvent $suite
     */
    public function beforeSuite(SuiteEvent $suite)
    {
        include __DIR__."/Html/Template/ReportHeader.html";
    }

    /**
     * @param SpecificationEvent $specification
     */
    public function beforeSpecification(SpecificationEvent $specification)
    {
        $index = $this->index++;
        $name = $specification->getTitle();
        include __DIR__."/Html/Template/ReportSpecificationStarts.html";
    }

    /**
     * @param SpecificationEvent $specification
     */
    public function afterSpecification(SpecificationEvent $specification)
    {
        include __DIR__."/Html/Template/ReportSpecificationEnds.html";
    }

    /**
     * @param ExampleEvent $event
     */
    public function afterExample(ExampleEvent $event)
    {
        $reportLine = $this->reportItemFactory->create($event, $this->getPresenter());
        $reportLine->write($this->index - 1);
        $this->getIO()->write(PHP_EOL);
    }

    /**
     * @param SuiteEvent $suite
     */
    public function afterSuite(SuiteEvent $suite)
    {
        include __DIR__."/Html/Template/ReportSummary.html";
        include __DIR__."/Html/Template/ReportFooter.html";
    }
}
