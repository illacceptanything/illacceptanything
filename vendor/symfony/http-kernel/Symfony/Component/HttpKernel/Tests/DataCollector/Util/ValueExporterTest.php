<?php

/*
 * This file is part of the Symfony package.
 *
 * (c) Fabien Potencier <fabien@symfony.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace Symfony\Component\HttpKernel\Tests\DataCollector\Util;

use Symfony\Component\HttpKernel\DataCollector\Util\ValueExporter;

class ValueExporterTest extends \PHPUnit_Framework_TestCase
{
    /**
     * @var ValueExporter
     */
    private $valueExporter;

    protected function setUp()
    {
        $this->valueExporter = new ValueExporter();
    }

    public function testDateTime()
    {
        $dateTime = new \DateTime('2014-06-10 07:35:40', new \DateTimeZone('UTC'));
        $this->assertSame('Object(DateTime) - 2014-06-10T07:35:40+0000', $this->valueExporter->exportValue($dateTime));
    }

    public function testDateTimeImmutable()
    {
        if (!class_exists('DateTimeImmutable', false)) {
            $this->markTestSkipped('Test skipped, class DateTimeImmutable does not exist.');
        }

        $dateTime = new \DateTimeImmutable('2014-06-10 07:35:40', new \DateTimeZone('UTC'));
        $this->assertSame('Object(DateTimeImmutable) - 2014-06-10T07:35:40+0000', $this->valueExporter->exportValue($dateTime));
    }
}
