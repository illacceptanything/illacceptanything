<?php

namespace Fake;

use Symfony\Component\Console\Helper\DialogHelper as BaseDialogHelper;
use Symfony\Component\Console\Output\OutputInterface;

class DialogHelper extends BaseDialogHelper
{
    private $answer;
    private $hasBeenAsked = false;

    public function setAnswer($answer)
    {
        $this->answer = $answer;
    }

    public function askConfirmation(OutputInterface $output, $question, $default = true)
    {
        $this->hasBeenAsked = true;
        return (bool)$this->answer;
    }

    public function hasBeenAsked()
    {
        return $this->hasBeenAsked;
    }
}
