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

namespace PhpSpec\Event;

use PhpSpec\Loader\Suite;
use Symfony\Component\EventDispatcher\Event;

/**
 * Class SuiteEvent holds information about the suite event
 */
class SuiteEvent extends Event implements EventInterface
{
    /**
     * @var Suite
     */
    private $suite;

    /**
     * @var float
     */
    private $time;

    /**
     * @var integer
     */
    private $result;

    /**
     * @var boolean
     */
    private $worthRerunning = false;

    /**
     * @param Suite   $suite
     * @param float   $time
     * @param integer $result
     */
    public function __construct(Suite $suite, $time = null, $result = null)
    {
        $this->suite  = $suite;
        $this->time   = $time;
        $this->result = $result;
    }

    /**
     * @return Suite
     */
    public function getSuite()
    {
        return $this->suite;
    }

    /**
     * @return float
     */
    public function getTime()
    {
        return $this->time;
    }

    /**
     * @return integer
     */
    public function getResult()
    {
        return $this->result;
    }

    /**
     * @return bool
     */
    public function isWorthRerunning()
    {
        return $this->worthRerunning;
    }

    public function markAsWorthRerunning()
    {
        $this->worthRerunning = true;
    }
}
