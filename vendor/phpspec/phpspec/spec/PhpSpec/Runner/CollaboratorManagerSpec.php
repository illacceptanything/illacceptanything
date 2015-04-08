<?php

namespace spec\PhpSpec\Runner;

use PhpSpec\ObjectBehavior;
use Prophecy\Argument;

use PhpSpec\Formatter\Presenter\PresenterInterface;

use ReflectionFunction;
use ReflectionParameter;

class CollaboratorManagerSpec extends ObjectBehavior
{
    function let(PresenterInterface $presenter)
    {
        $this->beConstructedWith($presenter);
    }

    function it_stores_collaborators_by_name($collaborator)
    {
        $this->set('custom_collaborator', $collaborator);
        $this->get('custom_collaborator')->shouldReturn($collaborator);
    }

    function it_provides_a_method_to_check_if_collaborator_exists($collaborator)
    {
        $this->set('custom_collaborator', $collaborator);

        $this->has('custom_collaborator')->shouldReturn(true);
        $this->has('nonexistent')->shouldReturn(false);
    }

    function it_throws_CollaboratorException_on_attempt_to_get_unexisting_collaborator()
    {
        $this->shouldThrow('PhpSpec\Exception\Wrapper\CollaboratorException')
            ->duringGet('nonexistent');
    }

    function it_creates_function_arguments_for_ReflectionFunction(
        ReflectionFunction $function, ReflectionParameter $param1, ReflectionParameter $param2
    ) {
        $this->set('arg1', '123');
        $this->set('arg2', '456');
        $this->set('arg3', '789');

        $function->getParameters()->willReturn(array($param1, $param2));
        $param1->getName()->willReturn('arg1');
        $param2->getName()->willReturn('arg3');

        $this->getArgumentsFor($function)->shouldReturn(array('123', '789'));
    }

    function it_creates_null_function_arguments_for_ReflectionFunction_if_no_collaborator_found(
        ReflectionFunction $function, ReflectionParameter $param1, ReflectionParameter $param2
    ) {
        $this->set('arg1', '123');
        $this->set('arg2', '456');
        $this->set('arg3', '789');

        $function->getParameters()->willReturn(array($param1, $param2));
        $param1->getName()->willReturn('arg4');
        $param2->getName()->willReturn('arg3');

        $this->getArgumentsFor($function)->shouldReturn(array(null, '789'));
    }
}
