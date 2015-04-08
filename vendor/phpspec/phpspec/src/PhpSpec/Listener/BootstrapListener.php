<?php

namespace PhpSpec\Listener;

use PhpSpec\Console\IO;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

class BootstrapListener implements EventSubscriberInterface
{
    /**
     * @var IO
     */
    private $io;

    public function __construct(IO $io)
    {
        $this->io = $io;
    }

    public static function getSubscribedEvents()
    {
        return array('beforeSuite' => 'beforeSuite');
    }

    public function beforeSuite()
    {
        if ($bootstrap = $this->io->getBootstrapPath()) {
            if (!is_file($bootstrap)) {
                throw new \RuntimeException(sprintf("Bootstrap file '%s' does not exist", $bootstrap));
            }

            require $bootstrap;
        }
    }
}
