<?php

/*
 * This file is part of Psy Shell
 *
 * (c) 2012-2014 Justin Hileman
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace Psy\CodeCleaner;

use PhpParser\Node;
use PhpParser\Node\Expr;
use PhpParser\Node\Expr\ClassConstFetch;
use PhpParser\Node\Expr\ConstFetch;
use Psy\Exception\FatalErrorException;

/**
 * Validate that namespaced constant references will succeed.
 *
 * This pass throws a FatalErrorException rather than letting PHP run
 * headfirst into a real fatal error and die.
 *
 * @todo Detect constants defined in the current code snippet?
 *       ... Might not be worth it, since it would need to both be defining and
 *       referencing a namespaced constant, which doesn't seem like that big of
 *       a target for failure.
 */
class ValidConstantPass extends NamespaceAwarePass
{
    /**
     * Validate that namespaced constant references will succeed.
     *
     * Note that this does not (yet) detect constants defined in the current code
     * snippet. It won't happen very often, so we'll punt for now.
     *
     * @throws FatalErrorException if a constant reference is not defined.
     *
     * @param Node $node
     */
    public function leaveNode(Node $node)
    {
        if ($node instanceof ConstFetch && count($node->name->parts) > 1) {
            $name = $this->getFullyQualifiedName($node->name);
            if (!defined($name)) {
                throw new FatalErrorException(sprintf('Undefined constant %s', $name), 0, 1, null, $node->getLine());
            }
        } elseif ($node instanceof ClassConstFetch) {
            $this->validateClassConstFetchExpression($node);
        }
    }

    /**
     * Validate a class constant fetch expression.
     *
     * @throws FatalErrorException if a class constant is not defined.
     *
     * @param ClassConstFetch $stmt
     */
    protected function validateClassConstFetchExpression(ClassConstFetch $stmt)
    {
        // if class name is an expression, give it a pass for now
        if (!$stmt->class instanceof Expr) {
            $className = $this->getFullyQualifiedName($stmt->class);

            // if the class doesn't exist, don't throw an exception… it might be
            // defined in the same line it's used or something stupid like that.
            if (class_exists($className)) {
                $constName = sprintf('%s::%s', $className, $stmt->name);
                if (!defined($constName)) {
                    $msg = sprintf('Class constant \'%s\' not found', $constName);
                    throw new FatalErrorException($msg, 0, 1, null, $stmt->getLine());
                }
            }
        }
    }
}
