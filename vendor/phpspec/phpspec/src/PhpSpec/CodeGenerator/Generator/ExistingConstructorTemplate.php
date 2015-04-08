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

namespace PhpSpec\CodeGenerator\Generator;

use PhpSpec\CodeGenerator\TemplateRenderer;
use ReflectionMethod;

class ExistingConstructorTemplate
{
    private $templates;
    private $class;
    private $className;
    private $arguments;
    private $methodName;

    /**
     * @param TemplateRenderer $templates
     * @param string           $class
     * @param string           $className
     * @param array            $arguments
     * @param string           $methodName
     */
    public function __construct(TemplateRenderer $templates, $methodName, array $arguments, $className, $class)
    {
        $this->templates  = $templates;
        $this->class      = $class;
        $this->className  = $className;
        $this->arguments  = $arguments;
        $this->methodName = $methodName;
    }

    /**
     * @return string
     */
    public function getContent()
    {
        if (!$this->numberOfConstructorArgumentsMatchMethod()) {
            return $this->getExceptionContent();
        }

        return $this->getCreateObjectContent();
    }

    /**
     * @return bool
     */
    private function numberOfConstructorArgumentsMatchMethod()
    {
        $constructorArguments = 0;

        $constructor = new ReflectionMethod($this->class, '__construct');
        $params = $constructor->getParameters();

        foreach ($params as $param) {
            if (!$param->isOptional()) {
                $constructorArguments++;
            }
        }

        return $constructorArguments == count($this->arguments);
    }

    /**
     * @return string
     */
    private function getExceptionContent()
    {
        $values = $this->getValues();

        if (!$content = $this->templates->render('named_constructor_exception', $values)) {
            $content = $this->templates->renderString(
                $this->getExceptionTemplate(), $values
            );
        }

        return $content;
    }

    /**
     * @return string
     */
    private function getCreateObjectContent()
    {
        $values = $this->getValues(true);

        if (!$content = $this->templates->render('named_constructor_create_object', $values)) {
            $content = $this->templates->renderString(
                $this->getCreateObjectTemplate(), $values
            );
        }

        return $content;
    }

    /**
     * @param  bool  $constructorArguments
     * @return array
     */
    private function getValues($constructorArguments = false)
    {
        $argString = count($this->arguments)
            ? '$argument'.implode(', $argument',  range(1, count($this->arguments)))
            : ''
        ;

        return array(
            '%methodName%'           => $this->methodName,
            '%arguments%'            => $argString,
            '%returnVar%'            => '$'.lcfirst($this->className),
            '%className%'            => $this->className,
            '%constructorArguments%' => $constructorArguments ? $argString : ''
        );
    }

    /**
     * @return string
     */
    private function getCreateObjectTemplate()
    {
        return file_get_contents(__DIR__.'/templates/named_constructor_create_object.template');
    }

    /**
     * @return string
     */
    private function getExceptionTemplate()
    {
        return file_get_contents(__DIR__.'/templates/named_constructor_exception.template');
    }
}
