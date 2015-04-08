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

namespace PhpSpec\Runner\Maintainer;

use PhpSpec\Loader\Node\ExampleNode;
use PhpSpec\SpecificationInterface;
use PhpSpec\Runner\MatcherManager;
use PhpSpec\Runner\CollaboratorManager;
use PhpSpec\Wrapper\Collaborator;
use PhpSpec\Wrapper\Unwrapper;
use Prophecy\Prophet;

class CollaboratorsMaintainer implements MaintainerInterface
{
    /**
     * @var string
     */
    private static $docex = '#@param *([^ ]*) *\$([^ ]*)#';
    /**
     * @var \PhpSpec\Wrapper\Unwrapper
     */
    private $unwrapper;
    /**
     * @var Prophet
     */
    private $prophet;

    /**
     * @param Unwrapper $unwrapper
     */
    public function __construct(Unwrapper $unwrapper)
    {
        $this->unwrapper = $unwrapper;
    }

    /**
     * @param ExampleNode $example
     *
     * @return bool
     */
    public function supports(ExampleNode $example)
    {
        return true;
    }

    /**
     * @param ExampleNode            $example
     * @param SpecificationInterface $context
     * @param MatcherManager         $matchers
     * @param CollaboratorManager    $collaborators
     */
    public function prepare(ExampleNode $example, SpecificationInterface $context,
                            MatcherManager $matchers, CollaboratorManager $collaborators)
    {
        $this->prophet = new Prophet(null, $this->unwrapper, null);

        $classRefl = $example->getSpecification()->getClassReflection();

        if ($classRefl->hasMethod('let')) {
            $this->generateCollaborators($collaborators, $classRefl->getMethod('let'));
        }

        $this->generateCollaborators($collaborators, $example->getFunctionReflection());
    }

    /**
     * @param ExampleNode            $example
     * @param SpecificationInterface $context
     * @param MatcherManager         $matchers
     * @param CollaboratorManager    $collaborators
     */
    public function teardown(ExampleNode $example, SpecificationInterface $context,
                             MatcherManager $matchers, CollaboratorManager $collaborators)
    {
        $this->prophet->checkPredictions();
    }

    /**
     * @return int
     */
    public function getPriority()
    {
        return 50;
    }

    /**
     * @param CollaboratorManager         $collaborators
     * @param \ReflectionFunctionAbstract $function
     */
    private function generateCollaborators(CollaboratorManager $collaborators, \ReflectionFunctionAbstract $function)
    {
        if ($comment = $function->getDocComment()) {
            $comment = str_replace("\r\n", "\n", $comment);
            foreach (explode("\n", trim($comment)) as $line) {
                if (preg_match(self::$docex, $line, $match)) {
                    $collaborator = $this->getOrCreateCollaborator($collaborators, $match[2]);
                    $collaborator->beADoubleOf($match[1]);
                }
            }
        }

        foreach ($function->getParameters() as $parameter) {
            $collaborator = $this->getOrCreateCollaborator($collaborators, $parameter->getName());
            if (null !== $class = $parameter->getClass()) {
                $collaborator->beADoubleOf($class->getName());
            }
        }
    }

    /**
     * @param CollaboratorManager $collaborators
     * @param string              $name
     *
     * @return Collaborator
     */
    private function getOrCreateCollaborator(CollaboratorManager $collaborators, $name)
    {
        if (!$collaborators->has($name)) {
            $collaborator = new Collaborator($this->prophet->prophesize());
            $collaborators->set($name, $collaborator);
        }

        return $collaborators->get($name);
    }
}
