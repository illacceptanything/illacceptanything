<?php

namespace spec\PhpSpec\Formatter\Html;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Event\ExampleEvent;
use PhpSpec\Formatter\Template;
use PhpSpec\Formatter\Presenter\PresenterInterface as Presenter;

class ReportItemFactorySpec extends ObjectBehavior
{
    function let(Template $template)
    {
        $this->beConstructedWith($template);
    }

    function it_creates_a_ReportPassedItem(ExampleEvent $event, Presenter $presenter)
    {
        $event->getResult()->willReturn(ExampleEvent::PASSED);
        $this->create($event, $presenter)->shouldHaveType('PhpSpec\Formatter\Html\ReportPassedItem');
    }

    function it_creates_a_ReportPendingItem(ExampleEvent $event, Presenter $presenter)
    {
        $event->getResult()->willReturn(ExampleEvent::PENDING);
        $this->create($event, $presenter)->shouldHaveType('PhpSpec\Formatter\Html\ReportPendingItem');
    }

    function it_creates_a_ReportFailedItem(ExampleEvent $event, Presenter $presenter)
    {
        $event->getResult()->willReturn(ExampleEvent::FAILED);
        $this->create($event, $presenter)->shouldHaveType('PhpSpec\Formatter\Html\ReportFailedItem');
    }

    function it_creates_a_ReportBrokenItem(ExampleEvent $event, Presenter $presenter)
    {
        $event->getResult()->willReturn(ExampleEvent::BROKEN);
        $this->create($event, $presenter)->shouldHaveType('PhpSpec\Formatter\Html\ReportFailedItem');
    }
}
