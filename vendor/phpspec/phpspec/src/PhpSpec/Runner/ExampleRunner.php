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

namespace PhpSpec\Runner;

use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use PhpSpec\Runner\Maintainer\LetAndLetgoMaintainer;
use PhpSpec\Formatter\Presenter\PresenterInterface;
use PhpSpec\SpecificationInterface;
use PhpSpec\Event\ExampleEvent;
use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\Exception\Exception as PhpSpecException;
use PhpSpec\Exception\Example as ExampleException;
use Prophecy\Exception as ProphecyException;
use Exception;

class ExampleRunner
{
    /**
     * @var \Symfony\Component\EventDispatcher\EventDispatcherInterface
     */
    private $dispatcher;
    /**
     * @var \PhpSpec\Formatter\Presenter\PresenterInterface
     */
    private $presenter;
    /**
     * @var Maintainer\MaintainerInterface[]
     */
    private $maintainers = array();

    /**
     * @param EventDispatcherInterface $dispatcher
     * @param PresenterInterface       $presenter
     */
    public function __construct(EventDispatcherInterface $dispatcher, PresenterInterface $presenter)
    {
        $this->dispatcher = $dispatcher;
        $this->presenter  = $presenter;
    }

    /**
     * @param Maintainer\MaintainerInterface $maintainer
     */
    public function registerMaintainer(Maintainer\MaintainerInterface $maintainer)
    {
        $this->maintainers[] = $maintainer;

        @usort($this->maintainers, function ($maintainer1, $maintainer2) {
            return $maintainer2->getPriority() - $maintainer1->getPriority();
        });
    }

    /**
     * @param ExampleNode $example
     *
     * @return int
     */
    public function run(ExampleNode $example)
    {
        $startTime = microtime(true);
        $this->dispatcher->dispatch('beforeExample',
            new ExampleEvent($example)
        );

        try {
            $this->executeExample(
                $example->getSpecification()->getClassReflection()->newInstance(),
                $example
            );

            $status    = ExampleEvent::PASSED;
            $exception = null;
        } catch (ExampleException\PendingException $e) {
            $status    = ExampleEvent::PENDING;
            $exception = $e;
        } catch (ExampleException\SkippingException $e) {
            $status    = ExampleEvent::SKIPPED;
            $exception = $e;
        } catch (ProphecyException\Prediction\PredictionException $e) {
            $status    = ExampleEvent::FAILED;
            $exception = $e;
        } catch (ExampleException\FailureException $e) {
            $status    = ExampleEvent::FAILED;
            $exception = $e;
        } catch (Exception $e) {
            $status    = ExampleEvent::BROKEN;
            $exception = $e;
        }

        if ($exception instanceof PhpSpecException) {
            $exception->setCause($example->getFunctionReflection());
        }

        $runTime = microtime(true) - $startTime;
        $this->dispatcher->dispatch('afterExample',
            $event = new ExampleEvent($example, $runTime, $status, $exception)
        );

        return $event->getResult();
    }

    /**
     * @param SpecificationInterface $context
     * @param ExampleNode            $example
     *
     * @throws \PhpSpec\Exception\Example\PendingException
     * @throws \Exception
     */
    protected function executeExample(SpecificationInterface $context, ExampleNode $example)
    {
        if ($example->isPending()) {
            throw new ExampleException\PendingException();
        }

        $matchers      = new MatcherManager($this->presenter);
        $collaborators = new CollaboratorManager($this->presenter);
        $maintainers   = array_filter($this->maintainers, function ($maintainer) use ($example) {
            return $maintainer->supports($example);
        });

        // run maintainers prepare
        foreach ($maintainers as $maintainer) {
            $maintainer->prepare($example, $context, $matchers, $collaborators);
        }

        // execute example
        $reflection = $example->getFunctionReflection();

        try {
            $reflection->invokeArgs($context, $collaborators->getArgumentsFor($reflection));
        } catch (\Exception $e) {
            $this->runMaintainersTeardown(
                $this->searchExceptionMaintainers($maintainers),
                $example,
                $context,
                $matchers,
                $collaborators
            );
            throw $e;
        }

        $this->runMaintainersTeardown($maintainers, $example, $context, $matchers, $collaborators);
    }

    /**
     * @param Maintainer\MaintainerInterface[] $maintainers
     * @param ExampleNode                      $example
     * @param SpecificationInterface           $context
     * @param MatcherManager                   $matchers
     * @param CollaboratorManager              $collaborators
     */
    private function runMaintainersTeardown(array $maintainers, ExampleNode $example, SpecificationInterface $context, MatcherManager $matchers, CollaboratorManager $collaborators)
    {
        foreach (array_reverse($maintainers) as $maintainer) {
            $maintainer->teardown($example, $context, $matchers, $collaborators);
        }
    }

    /**
     * @param Maintainer\MaintainerInterface[] $maintainers
     *
     * @return Maintainer\MaintainerInterface[]
     */
    private function searchExceptionMaintainers(array $maintainers)
    {
        return array_filter(
            $maintainers,
            function ($maintainer) {
                return $maintainer instanceof LetAndLetgoMaintainer;
            }
        );
    }
}
