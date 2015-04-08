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

namespace PhpSpec\Console;

use PhpSpec\IO\IOInterface;
use Symfony\Component\Console\Helper\DialogHelper;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;
use PhpSpec\Config\OptionsConfig;

/**
 * Class IO deals with input and output from command line interaction
 */
class IO implements IOInterface
{
    const COL_MIN_WIDTH = 40;
    const COL_DEFAULT_WIDTH = 60;
    const COL_MAX_WIDTH = 80;

    /**
     * @var \Symfony\Component\Console\Input\InputInterface
     */
    private $input;

    /**
     * @var \Symfony\Component\Console\Output\OutputInterface
     */
    private $output;

    /**
     * @var \Symfony\Component\Console\Helper\DialogHelper
     */
    private $dialogHelper;

    /**
     * @var string
     */
    private $lastMessage;

    /**
     * @var bool
     */
    private $hasTempString = false;

    /**
      * @var OptionsConfig
      */
    private $config;

    /**
     * @var integer
     */
    private $consoleWidth;

    /**
     * @param InputInterface  $input
     * @param OutputInterface $output
     * @param DialogHelper    $dialogHelper
     * @param OptionsConfig   $config
     */
    public function __construct(InputInterface $input, OutputInterface $output, DialogHelper $dialogHelper, OptionsConfig $config)
    {
        $this->input   = $input;
        $this->output  = $output;
        $this->dialogHelper = $dialogHelper;
        $this->config  = $config;
    }

    /**
     * @return bool
     */
    public function isInteractive()
    {
        return $this->input->isInteractive();
    }

    /**
     * @return bool
     */
    public function isDecorated()
    {
        return $this->output->isDecorated();
    }

    /**
     * @return bool
     */
    public function isCodeGenerationEnabled()
    {
        if (!$this->isInteractive()) {
            return false;
        }

        return $this->config->isCodeGenerationEnabled()
            && !$this->input->getOption('no-code-generation');
    }

    /**
     * @return bool
     */
    public function isStopOnFailureEnabled()
    {
        return $this->config->isStopOnFailureEnabled()
            || $this->input->getOption('stop-on-failure');
    }

    /**
     * @return bool
     */
    public function isVerbose()
    {
        return OutputInterface::VERBOSITY_VERBOSE <= $this->output->getVerbosity();
    }

    /**
     * @return string
     */
    public function getLastWrittenMessage()
    {
        return $this->lastMessage;
    }

    /**
     * @param string       $message
     * @param integer|null $indent
     */
    public function writeln($message = '', $indent = null)
    {
        $this->write($message, $indent, true);
    }

    /**
     * @param string       $message
     * @param integer|null $indent
     */
    public function writeTemp($message, $indent = null)
    {
        $this->write($message, $indent);
        $this->hasTempString = true;
    }

    /**
     * @return null|string
     */
    public function cutTemp()
    {
        if (false === $this->hasTempString) {
            return;
        }

        $message = $this->lastMessage;
        $this->write('');

        return $message;
    }

    /**
     *
     */
    public function freezeTemp()
    {
        $this->write($this->lastMessage);
    }

    /**
     * @param string       $message
     * @param integer|null $indent
     * @param bool         $newline
     */
    public function write($message, $indent = null, $newline = false)
    {
        if ($this->hasTempString) {
            $this->hasTempString = false;
            $this->overwrite($message, $indent, $newline);

            return;
        }

        if (null !== $indent) {
            $message = $this->indentText($message, $indent);
        }

        $this->output->write($message, $newline);
        $this->lastMessage = $message.($newline ? "\n" : '');
    }

    /**
     * @param string       $message
     * @param integer|null $indent
     */
    public function overwriteln($message = '', $indent = null)
    {
        $this->overwrite($message, $indent, true);
    }

    /**
     * @param string       $message
     * @param integer|null $indent
     * @param bool         $newline
     */
    public function overwrite($message, $indent = null, $newline = false)
    {
        if (null !== $indent) {
            $message = $this->indentText($message, $indent);
        }

        if ($message === $this->lastMessage) {
            return;
        }

        $commonPrefix = $this->getCommonPrefix($message, $this->lastMessage);
        $newSuffix = substr($message, strlen($commonPrefix));
        $oldSuffix = substr($this->lastMessage, strlen($commonPrefix));

        $overwriteLength = strlen(strip_tags($oldSuffix));

        $this->write(str_repeat("\x08", $overwriteLength));
        $this->write($newSuffix);

        $fill = $overwriteLength - strlen(strip_tags($newSuffix));
        if ($fill > 0) {
            $this->write(str_repeat(' ', $fill));
            $this->write(str_repeat("\x08", $fill));
        }

        if ($newline) {
            $this->writeln();
        }

        $this->lastMessage = $message.($newline ? "\n" : '');
    }

    private function getCommonPrefix($stringA, $stringB)
    {
        for ($i = 0; $i<min(strlen($stringA), strlen($stringB)); $i++) {
            if ($stringA[$i] != $stringB[$i]) {
                break;
            }
        }

        $common = substr($stringA, 0, $i);

        if (preg_match('/(^.*)<[a-z-]*>?[^<]*$/', $common, $matches)) {
            $common = $matches[1];
        }

        return $common;
    }

    /**
     * @param string      $question
     * @param string|null $default
     *
     * @return string
     */
    public function ask($question, $default = null)
    {
        return $this->dialogHelper->ask($this->output, $question, $default);
    }

    /**
     * @param string $question
     * @param bool   $default
     *
     * @return Boolean
     */
    public function askConfirmation($question, $default = true)
    {
        $lines   = array();
        $lines[] = '<question>'.str_repeat(' ', $this->getBlockWidth())."</question>";
        foreach (explode("\n", wordwrap($question, $this->getBlockWidth() - 4, "\n", true)) as $line) {
            $lines[] = '<question>  '.str_pad($line, $this->getBlockWidth() - 2).'</question>';
        }
        $lines[] = '<question>'.str_repeat(' ', $this->getBlockWidth() - 8).'</question> <value>'.
            ($default ? '[Y/n]' : '[y/N]').'</value> ';

        return $this->dialogHelper->askConfirmation(
            $this->output, implode("\n", $lines). "\n", $default
        );
    }

    /**
     * @param string      $question
     * @param callable    $validator
     * @param int|false   $attempts
     * @param string|null $default
     *
     * @return string
     */
    public function askAndValidate($question, $validator, $attempts = false, $default = null)
    {
        return $this->dialogHelper->askAndValidate($this->output, $question, $validator, $attempts, $default);
    }

    /**
     * @param string  $text
     * @param integer $indent
     *
     * @return string
     */
    private function indentText($text, $indent)
    {
        return implode("\n", array_map(
            function ($line) use ($indent) {
                return str_repeat(' ', $indent).$line;
            },
            explode("\n", $text)
        ));
    }

    public function isRerunEnabled()
    {
        return !$this->input->getOption('no-rerun') && $this->config->isReRunEnabled();
    }

    public function isFakingEnabled()
    {
        return $this->input->getOption('fake') || $this->config->isFakingEnabled();
    }

    public function getBootstrapPath()
    {
        if ($path = $this->input->getOption('bootstrap')) {
            return $path;
        }

        if ($path = $this->config->getBootstrapPath()) {
            return $path;
        }
        return false;
    }

    /**
     * @param integer $width
     */
    public function setConsoleWidth($width)
    {
        $this->consoleWidth = $width;
    }

    /**
     * @return integer
     */
    public function getBlockWidth()
    {
        $width = self::COL_DEFAULT_WIDTH;
        if ($this->consoleWidth && ($this->consoleWidth - 10) > self::COL_MIN_WIDTH) {
            $width = $this->consoleWidth - 10;
        }
        if ($width > self::COL_MAX_WIDTH) {
            $width = self::COL_MAX_WIDTH;
        }
        return $width;
    }
}
