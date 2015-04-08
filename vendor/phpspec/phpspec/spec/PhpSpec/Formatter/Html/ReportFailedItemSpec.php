<?php

namespace spec\PhpSpec\Formatter\Html;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Event\ExampleEvent;
use PhpSpec\Formatter\Presenter\PresenterInterface as Presenter;
use PhpSpec\Formatter\Html\Template;

class ReportFailedItemSpec extends ObjectBehavior
{
    const EVENT_TITLE = 'it does not works';
    const EVENT_MESSAGE = 'oops';
    static $BACKTRACE = array(
        array('line' => 42, 'file' => '/some/path/to/SomeException.php')
    );
    const BACKTRACE = "#42 /some/path/to/SomeException.php";
    const CODE = 'code';

    function let(Template $template, ExampleEvent $event, Presenter $presenter)
    {
        $this->beConstructedWith($template, $event, $presenter);
    }

    function it_writes_a_fail_message_for_a_failing_example(Template $template, ExampleEvent $event, Presenter $presenter)
    {
        $event->getTitle()->willReturn(self::EVENT_TITLE);
        $event->getMessage()->willReturn(self::EVENT_MESSAGE);
        $event->getBacktrace()->willReturn(self::$BACKTRACE);
        $event->getException()->willReturn(new \Exception());
        $template->render(Template::DIR.'/Template/ReportFailed.html', array(
            'title' => self::EVENT_TITLE,
            'message' => self::EVENT_MESSAGE,
            'backtrace' => self::BACKTRACE,
            'code' => self::CODE,
            'index' => 1,
            'specification' => 1
        ))->shouldBeCalled();
        $presenter->presentException(Argument::cetera())->willReturn(self::CODE);
        $this->write(1);
    }
}
