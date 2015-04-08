<?php

namespace spec\PhpSpec\Runner\Maintainer;

use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\Matcher\MatcherInterface;
use PhpSpec\ObjectBehavior;
use PhpSpec\Runner\CollaboratorManager;
use PhpSpec\Runner\MatcherManager;
use PhpSpec\SpecificationInterface;
use Prophecy\Argument;

class MatchersMaintainerSpec extends ObjectBehavior
{
    function it_should_add_default_matchers_to_the_matcher_manager(
        PresenterInterface $presenter, ExampleNode $example, SpecificationInterface $context,
        MatcherManager $matchers, CollaboratorManager $collaborators, MatcherInterface $matcher)
    {
        $this->beConstructedWith($presenter, array($matcher));
        $this->prepare($example, $context, $matchers, $collaborators);

        $matchers->replace(array($matcher))->shouldHaveBeenCalled();
    }
}
