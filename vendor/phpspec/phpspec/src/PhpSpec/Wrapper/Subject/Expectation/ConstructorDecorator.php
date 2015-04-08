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

namespace PhpSpec\Wrapper\Subject\Expectation;

use Exception;
use PhpSpec\Exception\Example\ErrorException;
use PhpSpec\Exception\Fracture\FractureException;
use PhpSpec\Util\Instantiator;
use PhpSpec\Wrapper\Subject\WrappedObject;

class ConstructorDecorator extends Decorator implements ExpectationInterface
{
    /**
     * @param ExpectationInterface $expectation
     */
    public function __construct(ExpectationInterface $expectation)
    {
        $this->setExpectation($expectation);
    }

    /**
     * @param string        $alias
     * @param mixed         $subject
     * @param array         $arguments
     * @param WrappedObject $wrappedObject
     *
     * @return boolean
     *
     * @throws \Exception
     * @throws \PhpSpec\Exception\Example\ErrorException
     * @throws \Exception
     * @throws \PhpSpec\Exception\Fracture\FractureException
     */
    public function match($alias, $subject, array $arguments = array(), WrappedObject $wrappedObject = null)
    {
        try {
            $wrapped = $subject->getWrappedObject();
        } catch (ErrorException $e) {
            throw $e;
        } catch (FractureException $e) {
            throw $e;
        } catch (Exception $e) {
            if (null !== $wrappedObject && $wrappedObject->getClassName()) {
                $instantiator = new Instantiator();
                $wrapped = $instantiator->instantiate($wrappedObject->getClassName());
            }
        }

        return $this->getExpectation()->match($alias, $wrapped, $arguments);
    }
}
