<?php

namespace spec\PhpSpec\Console;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;
use PhpSpec\Config\OptionsConfig;

use Symfony\Component\Console\Helper\DialogHelper;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

class IOSpec extends ObjectBehavior
{
    function let(InputInterface $input, OutputInterface $output, DialogHelper $dialogHelper, OptionsConfig $config)
    {
        $input->isInteractive()->willReturn(true);
        $input->getOption('no-code-generation')->willReturn(false);
        $input->getOption('stop-on-failure')->willReturn(false);

        $config->isCodeGenerationEnabled()->willReturn(true);
        $config->isStopOnFailureEnabled()->willReturn(false);

        $this->beConstructedWith($input, $output, $dialogHelper, $config);
    }

    function it_has_io_interface()
    {
        $this->shouldHaveType('PhpSpec\IO\IOInterface');
    }

    function it_is_code_generation_ready_if_no_input_config_says_otherwise()
    {
        $this->isCodeGenerationEnabled()->shouldReturn(true);
    }

    function it_is_not_code_generation_ready_if_input_is_not_interactive($input)
    {
        $input->isInteractive()->willReturn(false);

        $this->isCodeGenerationEnabled()->shouldReturn(false);
    }

    function it_is_not_code_generation_ready_if_command_line_option_is_set($input)
    {
        $input->getOption('no-code-generation')->willReturn(true);

        $this->isCodeGenerationEnabled()->shouldReturn(false);
    }

    function it_is_not_code_generation_ready_if_config_option_is_set($config)
    {
        $config->isCodeGenerationEnabled()->willReturn(false);

        $this->isCodeGenerationEnabled()->shouldReturn(false);
    }

    function it_will_not_stop_on_failure_if_no_input_config_says_otherwise()
    {
        $this->isStopOnFailureEnabled()->shouldReturn(false);
    }

    function it_will_stop_on_failure_if_command_line_option_is_set($input)
    {
        $input->getOption('stop-on-failure')->willReturn(true);

        $this->isStopOnFailureEnabled()->shouldReturn(true);
    }

    function it_will_stop_on_failure_if_config_option_is_set($config)
    {
        $config->isStopOnFailureEnabled()->willReturn(true);

        $this->isStopOnFailureEnabled()->shouldReturn(true);
    }

    function it_will_enable_rerunning_if_command_line_option_is_not_set_and_config_doesnt_disallow($input, $config)
    {
        $input->getOption('no-rerun')->willReturn(false);
        $config->isReRunEnabled()->willReturn(true);

        $this->isRerunEnabled()->shouldReturn(true);
    }

    function it_will_disable_rerunning_if_command_line_option_is_set($input, $config)
    {
        $input->getOption('no-rerun')->willReturn(true);
        $config->isReRunEnabled()->willReturn(true);

        $this->isRerunEnabled()->shouldReturn(false);
    }

    function it_will_disable_rerunning_if_config_option_is_set($input, $config)
    {
        $input->getOption('no-rerun')->willReturn(false);
        $config->isReRunEnabled()->willReturn(false);

        $this->isRerunEnabled()->shouldReturn(false);
    }

    function it_will_disable_faking_if_command_line_option_and_config_flag_are_not_set($input, $config)
    {
        $input->getOption('fake')->willReturn(false);
        $config->isFakingEnabled()->willReturn(false);

        $this->isFakingEnabled()->shouldReturn(false);
    }

    function it_will_enable_faking_if_command_line_option_is_set($input, $config)
    {
        $input->getOption('fake')->willReturn(true);
        $config->isFakingEnabled()->willReturn(false);

        $this->isFakingEnabled()->shouldReturn(true);
    }

    function it_will_enable_faking_if_config_flag_is_set($input, $config)
    {
        $input->getOption('fake')->willReturn(false);
        $config->isFakingEnabled()->willReturn(true);

        $this->isFakingEnabled()->shouldReturn(true);
    }

    function it_will_report_no_bootstrap_when_there_is_none($input, $config)
    {
        $input->getOption('bootstrap')->willReturn(null);
        $config->getBootstrapPath()->willReturn(false);

        $this->getBootstrapPath()->shouldReturn(false);
    }

    function it_will_report_bootstrap_path_when_one_is_in_the_config_file($input, $config)
    {
        $input->getOption('bootstrap')->willReturn(null);
        $config->getBootstrapPath()->willReturn('/path/to/bootstrap.php');

        $this->getBootstrapPath()->shouldReturn('/path/to/bootstrap.php');
    }

    function it_will_report_bootstrap_path_when_one_is_specified_at_the_command_line($input, $config)
    {
        $input->getOption('bootstrap')->willReturn('/path/to/bootstrap.php');
        $config->getBootstrapPath()->willReturn(false);

        $this->getBootstrapPath()->shouldReturn('/path/to/bootstrap.php');
    }

    function it_will_report_bootstrap_path_from_cli_when_different_paths_are_specified_in_config_and_cli($input, $config)
    {
        $input->getOption('bootstrap')->willReturn('/path/to/bootstrap.php');
        $config->getBootstrapPath()->willReturn('/path/to/different.php');

        $this->getBootstrapPath()->shouldReturn('/path/to/bootstrap.php');
    }

    function it_defaults_the_block_width()
    {
        $this->getBlockWidth()->shouldReturn(60);
    }

    function it_sets_the_block_width_to_the_minimum_when_terminal_is_narrow()
    {
        $this->setConsoleWidth(10);

        $this->getBlockWidth()->shouldReturn(60);
    }

    function it_sets_the_block_width_to_the_maximum_when_terminal_is_very_wide()
    {
        $this->setConsoleWidth(1000);

        $this->getBlockWidth()->shouldReturn(80);
    }

    function it_sets_the_block_width_to_narrower_than_the_terminal_width_when_terminal_is_in_range()
    {
        $this->setConsoleWidth(75);

        $this->getBlockWidth()->shouldReturn(65);
    }
}
