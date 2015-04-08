<?php

namespace ClassPreloader\Parser;

use PhpParser\NodeTraverser as BaseTraverser;

/**
 * This is the file node visitor class.
 *
 * This allows a filename to be set when visiting.
 */
class NodeTraverser extends BaseTraverser
{
    /**
     * Transverse the file.
     *
     * @param array  $nodes
     * @param string $filename
     *
     * @return void
     */
    public function traverseFile(array $nodes, $filename)
    {
        // Set the correct state on each visitor
        foreach ($this->visitors as $visitor) {
            if ($visitor instanceof AbstractNodeVisitor) {
                $visitor->setFilename($filename);
            }
        }

        return $this->traverse($nodes);
    }
}
