<?php

namespace spec\PhpSpec\Formatter;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\BasicFormatter;
use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\IO\IOInterface;
use PhpSpec\Listener\StatisticsCollector;

class BasicFormatterSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter, IOInterface $io, StatisticsCollector $stats)
    {
        $this->beAnInstanceOf('spec\PhpSpec\Formatter\TestableBasicFormatter');
        $this->beConstructedWith($presenter, $io, $stats);
    }

    function it_is_an_event_subscriber()
    {
        $this->shouldHaveType('Symfony\Component\EventDispatcher\EventSubscriberInterface');
    }

    function it_returns_a_list_of_subscribed_events()
    {
        $this::getSubscribedEvents()->shouldBe(
            array(
                'beforeSuite' => 'beforeSuite',
                'afterSuite' => 'afterSuite',
                'beforeExample' => 'beforeExample',
                'afterExample' => 'afterExample',
                'beforeSpecification' => 'beforeSpecification',
                'afterSpecification' => 'afterSpecification'
            )
        );
    }
}

class TestableBasicFormatter extends BasicFormatter
{
}
