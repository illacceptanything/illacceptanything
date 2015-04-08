<?php

namespace spec\PhpSpec\Formatter;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;
use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\IO\IOInterface;
use PhpSpec\Listener\StatisticsCollector;
use PhpSpec\Event\SpecificationEvent;
use PhpSpec\Event\ExampleEvent;
use PhpSpec\Loader\Node\SpecificationNode;
use PhpSpec\Event\SuiteEvent;
use PhpSpec\Exception\Example\SkippingException;

class JUnitFormatterSpec extends ObjectBehavior
{
    function let(
        PresenterInterface $presenter,
        IOInterface $io,
        StatisticsCollector $stats
    ) {
        $this->beConstructedWith($presenter, $io, $stats);
    }

    function it_is_an_event_subscriber()
    {
        $this->shouldHaveType('Symfony\Component\EventDispatcher\EventSubscriberInterface');
    }

    function it_stores_a_testcase_node_after_passed_example_run(
        ExampleEvent $event,
        SpecificationNode $specification,
        \ReflectionClass $refClass
    ) {
        $event->getResult()->willReturn(ExampleEvent::PASSED);
        $event->getTitle()->willReturn('example title');
        $event->getTime()->willReturn(1337);
        $event->getSpecification()->willReturn($specification);
        $specification->getClassReflection()->willReturn($refClass);
        $refClass->getName()->willReturn('Acme\Foo\Bar');

        $this->afterExample($event);

        $this->getTestCaseNodes()->shouldReturn(array(
            '<testcase name="example title" time="1337" classname="Acme\Foo\Bar" status="passed" />'
        ));
    }

    function it_stores_a_testcase_node_after_broken_example_run(
        ExampleEvent $event,
        SpecificationNode $specification,
        \ReflectionClass $refClass
    ) {
        $event->getResult()->willReturn(ExampleEvent::BROKEN);
        $event->getTitle()->willReturn('example title');
        $event->getTime()->willReturn(1337);

        $event->getException()->willReturn(new ExceptionStub('Something went wrong', 'Exception trace'));

        $event->getSpecification()->willReturn($specification);
        $specification->getClassReflection()->willReturn($refClass);
        $refClass->getName()->willReturn('Acme\Foo\Bar');

        $this->afterExample($event);

        $this->getTestCaseNodes()->shouldReturn(array(
            '<testcase name="example title" time="1337" classname="Acme\Foo\Bar" status="broken">'."\n".
                '<error type="spec\PhpSpec\Formatter\ExceptionStub" message="Something went wrong" />'."\n".
                '<system-err>'."\n".
                    '<![CDATA['."\n".
                        'Exception trace'."\n".
                    ']]>'."\n".
                '</system-err>'."\n".
            '</testcase>'
        ));
    }

    function it_stores_a_testcase_node_after_failed_example_run(
        ExampleEvent $event,
        SpecificationNode $specification,
        \ReflectionClass $refClass
    ) {
        $event->getResult()->willReturn(ExampleEvent::FAILED);
        $event->getTitle()->willReturn('example title');
        $event->getTime()->willReturn(1337);

        $event->getException()->willReturn(new ExceptionStub('Something went wrong', 'Exception trace'));

        $event->getSpecification()->willReturn($specification);
        $specification->getClassReflection()->willReturn($refClass);
        $refClass->getName()->willReturn('Acme\Foo\Bar');

        $this->afterExample($event);

        $this->getTestCaseNodes()->shouldReturn(array(
            '<testcase name="example title" time="1337" classname="Acme\Foo\Bar" status="failed">'."\n".
                '<failure type="spec\PhpSpec\Formatter\ExceptionStub" message="Something went wrong" />'."\n".
                '<system-err>'."\n".
                    '<![CDATA['."\n".
                        'Exception trace'."\n".
                    ']]>'."\n".
                '</system-err>'."\n".
            '</testcase>'
        ));
    }

    function it_stores_a_testcase_node_after_skipped_example_run(
        ExampleEvent $event,
        SpecificationNode $specification,
        \ReflectionClass $refClass
    ) {
        $event->getResult()->willReturn(ExampleEvent::SKIPPED);
        $event->getTitle()->willReturn('example title');
        $event->getTime()->willReturn(1337);

        $event->getException()->willReturn(new SkippingException('zog zog'));

        $event->getSpecification()->willReturn($specification);
        $specification->getClassReflection()->willReturn($refClass);
        $refClass->getName()->willReturn('Acme\Foo\Bar');

        $this->afterExample($event);

        // skipped tag is escaped because a skipped tag is also registered in the console formatter
        $this->getTestCaseNodes()->shouldReturn(array(
            '<testcase name="example title" time="1337" classname="Acme\Foo\Bar" status="skipped">'."\n".
                '\<skipped><![CDATA[ skipped: zog zog ]]>\</skipped>'."\n".
            '</testcase>'
        ));
    }

    function it_aggregates_testcase_nodes_and_store_them_after_specification_run(SpecificationEvent $event)
    {
        $event->getTitle()->willReturn('specification title');
        $event->getTime()->willReturn(42);

        $this->setTestCaseNodes(array(
            '<testcase name="example1" />',
            '<testcase name="example2" />',
            '<testcase name="example3" />',
        ));

        $this->setExampleStatusCounts(array(
            ExampleEvent::FAILED  => 1,
            ExampleEvent::BROKEN  => 2,
            ExampleEvent::PENDING => 5,
            ExampleEvent::SKIPPED => 3,
        ));
        $this->afterSpecification($event);

        $this->getTestSuiteNodes()->shouldReturn(array(
            '<testsuite name="specification title" time="42" tests="3" failures="1" errors="2" skipped="8">'."\n".
                '<testcase name="example1" />'."\n".
                '<testcase name="example2" />'."\n".
                '<testcase name="example3" />'."\n".
            '</testsuite>'
        ));
        $this->getTestCaseNodes()->shouldHaveCount(0);
        $this->getExampleStatusCounts()->shouldReturn(array(
            ExampleEvent::PASSED  => 0,
            ExampleEvent::PENDING => 0,
            ExampleEvent::SKIPPED => 0,
            ExampleEvent::FAILED  => 0,
            ExampleEvent::BROKEN  => 0,
        ));
    }

    function it_aggregates_testsuite_nodes_and_display_them_after_suite_run(SuiteEvent $event, $io, $stats)
    {
        $event->getTime()->willReturn(48151.62342);
        $stats->getFailedEvents()->willReturn(range(1, 12));
        $stats->getBrokenEvents()->willReturn(range(1, 3));
        $stats->getEventsCount()->willReturn(100);

        $this->setTestSuiteNodes(array(
            '<testsuite name="specification1" tests="3">'."\n".
                '<testcase name="example1" />'."\n".
                '<testcase name="example2" />'."\n".
                '<testcase name="example3" />'."\n".
            '</testsuite>',
            '<testsuite name="specification2" tests="2">'."\n".
                '<testcase name="example1" />'."\n".
                '<testcase name="example2" />'."\n".
            '</testsuite>'
        ));
        $this->afterSuite($event);

        $io->write(
            '<?xml version="1.0" encoding="UTF-8" ?>'."\n".
            '<testsuites time="48151.62342" tests="100" failures="12" errors="3">'."\n".
                '<testsuite name="specification1" tests="3">'."\n".
                    '<testcase name="example1" />'."\n".
                    '<testcase name="example2" />'."\n".
                    '<testcase name="example3" />'."\n".
                '</testsuite>'."\n".
                '<testsuite name="specification2" tests="2">'."\n".
                    '<testcase name="example1" />'."\n".
                    '<testcase name="example2" />'."\n".
                '</testsuite>'."\n".
            '</testsuites>'
        )->shouldBeCalled();
    }
}

class ExceptionStub
{
    protected $trace;
    protected $message;

    public function __construct($message, $trace)
    {
        $this->message = $message;
        $this->trace   = $trace;
    }

    public function getMessage()
    {
        return $this->message;
    }

    public function getTraceAsString()
    {
        return $this->trace;
    }
}
