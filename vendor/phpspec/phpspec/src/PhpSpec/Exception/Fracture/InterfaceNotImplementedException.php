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

namespace PhpSpec\Exception\Fracture;

/**
 * Class InterfaceNotImplementedException holds information about interface
 * not implemented exception
 */
class InterfaceNotImplementedException extends FractureException
{
    /**
     * @var mixed
     */
    private $subject;

    /**
     * @var string
     */
    private $interface;

    /**
     * @param string $message
     * @param mixed  $subject
     * @param string $interface
     */
    public function __construct($message, $subject, $interface)
    {
        parent::__construct($message);

        $this->subject   = $subject;
        $this->interface = $interface;
    }

    /**
     * @return mixed
     */
    public function getSubject()
    {
        return $this->subject;
    }

    /**
     * @return string
     */
    public function getInterface()
    {
        return $this->interface;
    }
}
