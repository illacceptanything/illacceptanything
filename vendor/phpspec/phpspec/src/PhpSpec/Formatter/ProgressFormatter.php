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

namespace PhpSpec\Formatter;

use PhpSpec\Console\IO;
use PhpSpec\Event\SuiteEvent;
use PhpSpec\Event\ExampleEvent;
use PhpSpec\Listener\StatisticsCollector;

class ProgressFormatter extends ConsoleFormatter
{
    const FPS = 10;

    private $lastDraw;

    public function afterExample(ExampleEvent $event)
    {
        $this->printException($event);

        $now = microtime(true);
        if (!$this->lastDraw || ($now - $this->lastDraw) > 1/self::FPS) {
            $this->lastDraw = $now;
            $this->drawStats();
        }
    }

    public function afterSuite(SuiteEvent $event)
    {
        $this->drawStats();

        $io = $this->getIO();
        $stats = $this->getStatisticsCollector();

        $io->freezeTemp();
        $io->writeln();

        $io->writeln(sprintf("%d specs", $stats->getTotalSpecs()));

        $counts = array();
        foreach ($stats->getCountsHash() as $type => $count) {
            if ($count) {
                $counts[] = sprintf('<%s>%d %s</%s>', $type, $count, $type, $type);
            }
        }
        $count = $stats->getEventsCount();
        $plural = $count !== 1 ? 's' : '';
        $io->write(sprintf("%d example%s ", $count, $plural));
        if (count($counts)) {
            $io->write(sprintf("(%s)", implode(', ', $counts)));
        }

        $io->writeln(sprintf("\n%sms", round($event->getTime() * 1000)));
        $io->writeln();
    }

    /**
     * @param $total
     * @param $counts
     * @return array
     */
    private function getPercentages($total, $counts)
    {
        return array_map(
            function ($count) use ($total) {
                if (0 == $total) {
                    return 0;
                }

                $percent = ($count == $total) ? 100 : $count / ($total / 100);

                return $percent == 0 || $percent > 1 ? floor($percent) : 1;
            },
            $counts
        );
    }

    /**
     * @param array $counts
     * @return array
     */
    private function getBarLengths($counts)
    {
        $stats = $this->getStatisticsCollector();
        $specProgress = ($stats->getTotalSpecsCount() == 0) ? 1 : ($stats->getTotalSpecs())/$stats->getTotalSpecsCount();
        $targetWidth = ceil($this->getIO()->getBlockWidth() * $specProgress);
        asort($counts);

        $barLengths = array_map(function($count) use ($targetWidth, $counts) {
            return $count ? max(1,round($targetWidth * $count / array_sum($counts))) : 0;
        }, $counts);

        return $barLengths;
    }

    /**
     * @param  array   $barLengths
     * @param  array   $percents
     * @param  boolean $isDecorated
     * @return array
     */
    private function formatProgressOutput($barLengths, $percents, $isDecorated)
    {
        $size = $this->getIO()->getBlockWidth();
        $progress = array();
        foreach ($barLengths as $status => $length) {
            $percent = $percents[$status];
            $text = $percent.'%';
            $length = ($size - $length) >= 0 ? $length : $size;
            $size = $size - $length;

            if ($isDecorated) {
                if ($length > strlen($text) + 2) {
                    $text = str_pad($text, $length, ' ', STR_PAD_BOTH);
                } else {
                    $text = str_pad('', $length, ' ');
                }

                $progress[$status] = sprintf("<$status-bg>%s</$status-bg>", $text);
            } else {
                $progress[$status] = str_pad(
                    sprintf('%s: %s', $status, $text),
                    15,
                    ' ',
                    STR_PAD_BOTH
                );
            }
        }
        krsort($progress);

        return $progress;
    }

    /**
     * @param IO    $io
     * @param array $progress
     * @param int   $total
     */
    private function updateProgressBar(IO $io, array $progress, $total)
    {
        if ($io->isDecorated()) {
            $progressBar = implode('', $progress);
            $pad = $this->getIO()->getBlockWidth() - strlen(strip_tags($progressBar));
            $io->writeTemp($progressBar.str_repeat(' ', $pad + 1).$total);
        } else {
            $io->writeTemp('/'.implode('/', $progress).'/  '.$total.' examples');
        }
    }

    private function drawStats()
    {
        $io = $this->getIO();
        $stats = $this->getStatisticsCollector();

        $percents = $this->getPercentages($stats->getEventsCount(), $stats->getCountsHash());
        $barLengths = $this->getBarLengths($stats->getCountsHash());
        $progress = $this->formatProgressOutput($barLengths, $percents, $io->isDecorated());

        $this->updateProgressBar($io, $progress, $stats->getEventsCount());
    }

    /**
     * @return float
     */
    private function getSpecProgress()
    {
        $stats = $this->getStatisticsCollector();
        $specProgress = $stats->getTotalSpecsCount() ? ($stats->getTotalSpecs() + 1) / $stats->getTotalSpecsCount() : 0;

        return $specProgress;
    }
}
