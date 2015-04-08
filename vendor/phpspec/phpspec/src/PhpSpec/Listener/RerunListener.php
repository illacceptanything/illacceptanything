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
use PhpSpec\Process\ReRunner;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

class RerunListener implements EventSubscriberInterface
{
    /**
     * @var ReRunner
     */
    private $reRunner;

    /**
     * @param ReRunner $reRunner
     */
    public function __construct(ReRunner $reRunner)
    {
        $this->reRunner = $reRunner;
    }

    /**
     * {@inheritdoc}
     */
    public static function getSubscribedEvents()
    {
        return array('afterSuite' => array('afterSuite', -1000));
    }

    /**
     * @param SuiteEvent $suiteEvent
     */
    public function afterSuite(SuiteEvent $suiteEvent)
    {
        if ($suiteEvent->isWorthRerunning()) {
            $this->reRunner->reRunSuite();
        }
    }
}
