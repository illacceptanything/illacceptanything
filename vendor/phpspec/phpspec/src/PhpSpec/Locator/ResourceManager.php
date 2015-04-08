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

use RuntimeException;

class ResourceManager implements ResourceManagerInterface
{
    /**
     * @var ResourceLocatorInterface[]
     */
    private $locators = array();

    /**
     * @param ResourceLocatorInterface $locator
     */
    public function registerLocator(ResourceLocatorInterface $locator)
    {
        $this->locators[] = $locator;

        @usort($this->locators, function ($locator1, $locator2) {
            return $locator2->getPriority() - $locator1->getPriority();
        });
    }

    /**
     * @param string $query
     *
     * @return ResourceInterface[]
     */
    public function locateResources($query)
    {
        $resources = array();
        foreach ($this->locators as $locator) {
            if (empty($query)) {
                $resources = array_merge($resources, $locator->getAllResources());
                continue;
            }

            if (!$locator->supportsQuery($query)) {
                continue;
            }

            $resources = array_merge($resources, $locator->findResources($query));
        }

        return $this->removeDuplicateResources($resources);
    }

    /**
     * @param string $classname
     *
     * @return ResourceInterface
     *
     * @throws \RuntimeException
     */
    public function createResource($classname)
    {
        foreach ($this->locators as $locator) {
            if ($locator->supportsClass($classname)) {
                return $locator->createResource($classname);
            }
        }

        throw new RuntimeException(sprintf(
            'Can not find appropriate suite scope for class `%s`.', $classname
        ));
    }

    /**
     * @param array $resources
     *
     * @return ResourceInterface[]
     */
    private function removeDuplicateResources(array $resources)
    {
        $filteredResources = array();

        foreach ($resources as $resource) {
            if (!array_key_exists($resource->getSpecClassname(), $filteredResources)) {
                $filteredResources[$resource->getSpecClassname()] = $resource;
            }
        }

        return array_values($filteredResources);
    }
}
