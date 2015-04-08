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

namespace PhpSpec\Formatter\Html;

use PhpSpec\Event\ExampleEvent;
use PhpSpec\Formatter\Presenter\PresenterInterface as Presenter;
use PhpSpec\Formatter\Template as TemplateInterface;

class ReportFailedItem
{
    /**
     * @var \PhpSpec\Formatter\Template
     */
    private $template;
    /**
     * @var \PhpSpec\Event\ExampleEvent
     */
    private $event;
    /**
     * @var int
     */
    private static $failingExamplesCount = 1;
    /**
     * @var \PhpSpec\Formatter\Presenter\PresenterInterface
     */
    private $presenter;

    /**
     * @param TemplateInterface $template
     * @param ExampleEvent      $event
     * @param Presenter         $presenter
     */
    public function __construct(TemplateInterface $template, ExampleEvent $event, Presenter $presenter)
    {
        $this->template = $template;
        $this->event = $event;
        $this->presenter = $presenter;
    }

    /**
     * @param int $index
     */
    public function write($index)
    {
        $code = $this->presenter->presentException($this->event->getException(), true);
        $this->template->render(Template::DIR.'/Template/ReportFailed.html',
            array(
                'title' => htmlentities(strip_tags($this->event->getTitle())),
                'message' => htmlentities(strip_tags($this->event->getMessage())),
                'backtrace' => $this->formatBacktrace(),
                'code' => $code,
                'index' => self::$failingExamplesCount++,
                'specification' => $index
            )
        );
    }

    /**
     * @return string
     */
    private function formatBacktrace()
    {
        $backtrace = '';
        foreach ($this->event->getBacktrace() as $step) {
            if (isset($step['line']) && isset($step['file'])) {
                $backtrace .= "#{$step['line']} {$step['file']}";
                $backtrace .= "<br />";
                $backtrace .= PHP_EOL;
            }
        }

        return rtrim($backtrace, "<br />".PHP_EOL);
    }
}
