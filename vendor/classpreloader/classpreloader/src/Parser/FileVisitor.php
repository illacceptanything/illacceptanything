<?php

namespace ClassPreloader\Parser;

use ClassPreloader\Exception\SkipFileException;
use PhpParser\Node;
use PhpParser\Node\Scalar\MagicConst\File;
use PhpParser\Node\Scalar\String;

/**
 * This is the file node visitor class.
 *
 * This is used to replace all references to __FILE__ with the actual file.
 */
class FileVisitor extends AbstractNodeVisitor
{
    /**
     * Should we skip the file if it contains a file constant?
     *
     * @var bool
     */
    protected $skip = false;

    /**
     * Create a new file visitor.
     *
     * @param bool $skip
     *
     * @return void
     */
    public function __construct($skip = false)
    {
        $this->skip = $skip;
    }

    /**
     * Enter and modify the node.
     *
     * @param \PhpParser\Node $node
     *
     * @return void
     */
    public function enterNode(Node $node)
    {
        if ($node instanceof File) {
            if ($this->skip) {
                throw new SkipFileException('__FILE__ constant found, skipping...');
            }

            return new String($this->getFilename());
        }
    }
}
