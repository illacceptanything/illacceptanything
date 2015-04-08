<?php

namespace spec\PhpSpec\Formatter\Html;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Event\ExampleEvent;
use PhpSpec\Formatter\Html\Template;

class ReportPendingItemSpec extends ObjectBehavior
{
    const EVENT_TITLE = 'it works';

    function let(Template $template, ExampleEvent $event)
    {
        $this->beConstructedWith($template, $event);
    }

    function it_writes_a_pass_message_for_a_passing_example(Template $template, ExampleEvent $event)
    {
        $event->getTitle()->willReturn(self::EVENT_TITLE);
        $template->render(Template::DIR.'/Template/ReportPending.html', array(
            'title' => self::EVENT_TITLE,
            'pendingExamplesCount' => 1
        ))->shouldBeCalled();
        $this->write();
    }
}
