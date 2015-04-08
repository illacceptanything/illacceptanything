<?php

namespace spec\PhpSpec\Formatter;

use PhpSpec\Console\IO;
use PhpSpec\Event\ExampleEvent;
use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\Listener\StatisticsCollector;
use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

class ProgressFormatterSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter, IO $io, StatisticsCollector $stats)
    {
        $this->beConstructedWith($presenter, $io, $stats);
    }

    function it_is_an_event_subscriber()
    {
        $this->shouldHaveType('Symfony\Component\EventDispatcher\EventSubscriberInterface');
    }

    function it_outputs_progress_as_0_when_0_examples_have_run(ExampleEvent $event, IO $io, StatisticsCollector $stats)
    {
        $stats->getEventsCount()->willReturn(0);
        $stats->getCountsHash()->willReturn(array(
                'passed'  => 0,
                'pending' => 0,
                'skipped' => 0,
                'failed'  => 0,
                'broken'  => 0,
            ));
        $stats->getTotalSpecs()->willReturn(0);
        $stats->getTotalSpecsCount()->willReturn(0);

        $this->afterExample($event);

        $expected = '/  skipped: 0%  /  pending: 0%  /  passed: 0%   /  failed: 0%   /  broken: 0%   /  0 examples';
        $io->writeTemp($expected)->shouldHaveBeenCalled();
    }

    function it_outputs_progress_as_0_when_0_examples_have_passed(ExampleEvent $event, IO $io, StatisticsCollector $stats)
    {
        $stats->getEventsCount()->willReturn(1);
        $stats->getCountsHash()->willReturn(array(
                'passed'  => 1,
                'pending' => 0,
                'skipped' => 0,
                'failed'  => 0,
                'broken'  => 0,
            ));
        $stats->getTotalSpecs()->willReturn(1);
        $stats->getTotalSpecsCount()->willReturn(1);

        $this->afterExample($event);

        $expected = '/  skipped: 0%  /  pending: 0%  / passed: 100%  /  failed: 0%   /  broken: 0%   /  1 examples';
        $io->writeTemp($expected)->shouldHaveBeenCalled();
    }

    function it_outputs_progress_as_100_when_1_of_3_examples_have_passed(ExampleEvent $event, IO $io, StatisticsCollector $stats)
    {
        $stats->getEventsCount()->willReturn(1);
        $stats->getCountsHash()->willReturn(array(
            'passed'  => 1,
            'pending' => 0,
            'skipped' => 0,
            'failed'  => 0,
            'broken'  => 0,
        ));
        $stats->getTotalSpecs()->willReturn(1);
        $stats->getTotalSpecsCount()->willReturn(3);

        $this->afterExample($event);

        $expected = '/  skipped: 0%  /  pending: 0%  / passed: 100%  /  failed: 0%   /  broken: 0%   /  1 examples';
        $io->writeTemp($expected)->shouldHaveBeenCalled();
    }

    function it_outputs_progress_as_33_when_3_of_3_examples_have_run_and_one_passed(ExampleEvent $event, IO $io, StatisticsCollector $stats)
    {
        $stats->getEventsCount()->willReturn(3);
        $stats->getCountsHash()->willReturn(array(
                'passed'  => 1,
                'pending' => 0,
                'skipped' => 0,
                'failed'  => 2,
                'broken'  => 0,
            ));
        $stats->getTotalSpecs()->willReturn(3);
        $stats->getTotalSpecsCount()->willReturn(3);

        $this->afterExample($event);

        $expected = '/  skipped: 0%  /  pending: 0%  /  passed: 33%  /  failed: 66%  /  broken: 0%   /  3 examples';
        $io->writeTemp($expected)->shouldHaveBeenCalled();
    }
}
