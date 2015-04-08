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

namespace PhpSpec\Locator;

interface ResourceLocatorInterface
{
    /**
     * @return ResourceInterface[]
     */
    public function getAllResources();

    /**
     * @param string $query
     *
     * @return boolean
     */
    public function supportsQuery($query);

    /**
     * @param string $query
     *
     * @return ResourceInterface[]
     */
    public function findResources($query);

    /**
     * @param string $classname
     *
     * @return boolean
     */
    public function supportsClass($classname);

    /**
     * @param string $classname
     *
     * @return ResourceInterface|null
     */
    public function createResource($classname);

    /**
     * @return integer
     */
    public function getPriority();
}
