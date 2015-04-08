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
use PhpSpec\Console\IO;
use PhpSpec\Locator\ResourceInterface;
use PhpSpec\Util\Filesystem;

class ReturnConstantGenerator implements GeneratorInterface
{
    /**
     * @var IO
     */
    private $io;
    /**
     * @var TemplateRenderer
     */
    private $templates;
    /**
     * @var Filesystem
     */
    private $filesystem;

    /**
     * @param IO               $io
     * @param TemplateRenderer $templates
     * @param Filesystem       $filesystem
     */
    public function __construct(IO $io, TemplateRenderer $templates, Filesystem $filesystem = null)
    {
        $this->io = $io;
        $this->templates = $templates;
        $this->filesystem = $filesystem ?: new Filesystem();
    }

    /**
     * @param ResourceInterface $resource
     * @param string            $generation
     * @param array             $data
     *
     * @return bool
     */
    public function supports(ResourceInterface $resource, $generation, array $data)
    {
        return 'returnConstant' == $generation;
    }

    /**
     * @param ResourceInterface $resource
     * @param array             $data
     */
    public function generate(ResourceInterface $resource, array $data)
    {
        $method = $data['method'];
        $expected = $data['expected'];

        $code = $this->filesystem->getFileContents($resource->getSrcFilename());

        $values = array('%constant%' => var_export($expected, true));
        if (!$content = $this->templates->render('method', $values)) {
            $content = $this->templates->renderString(
                $this->getTemplate(), $values
            );
        }

        $pattern = '/'.'(function\s+'.preg_quote($method, '/').'\s*\([^\)]*\))\s+{[^}]*?}/';
        $replacement = '$1'.$content;

        $modifiedCode = preg_replace($pattern, $replacement, $code);

        $this->filesystem->putFileContents($resource->getSrcFilename(), $modifiedCode);

        $this->io->writeln(sprintf(
            "<info>Method <value>%s::%s()</value> has been modified.</info>\n",
            $resource->getSrcClassname(), $method
        ), 2);
    }

    /**
     * @return int
     */
    public function getPriority()
    {
        return 0;
    }

    /**
     * @return string
     */
    protected function getTemplate()
    {
        return file_get_contents(__DIR__.'/templates/returnconstant.template');
    }
}
