<?php

/*
 * This file is part of the Symfony package.
 *
 * (c) Fabien Potencier <fabien@symfony.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace Symfony\Component\Translation\Tests;

use Symfony\Component\Translation\Translator;
use Symfony\Component\Translation\LoggingTranslator;
use Symfony\Component\Translation\Loader\ArrayLoader;

class LoggingTranslatorTest extends \PHPUnit_Framework_TestCase
{
    protected function setUp()
    {
        if (!interface_exists('Psr\Log\LoggerInterface')) {
            $this->markTestSkipped('The "LoggerInterface" is not available');
        }
    }

    public function testTransWithNoTranslationIsLogged()
    {
        $logger = $this->getMock('Psr\Log\LoggerInterface');
        $logger->expects($this->exactly(2))
            ->method('warning')
            ->with('Translation not found.')
        ;

        $translator = new Translator('ar');
        $loggableTranslator = new LoggingTranslator($translator, $logger);
        $loggableTranslator->transChoice('some_message2', 10, array('%count%' => 10));
        $loggableTranslator->trans('bar');
    }

    public function testTransChoiceFallbackIsLogged()
    {
        $logger = $this->getMock('Psr\Log\LoggerInterface');
        $logger->expects($this->once())
            ->method('debug')
            ->with('Translation use fallback catalogue.')
        ;

        $translator = new Translator('ar');
        $translator->setFallbackLocales(array('en'));
        $translator->addLoader('array', new ArrayLoader());
        $translator->addResource('array', array('some_message2' => 'one thing|%count% things'), 'en');
        $loggableTranslator = new LoggingTranslator($translator, $logger);
        $loggableTranslator->transChoice('some_message2', 10, array('%count%' => 10));
    }
}
