<?php

namespace spec\PhpSpec\Locator\PSR0;

use PhpSpec\ObjectBehavior;
use PhpSpec\Util\Filesystem;

use SplFileInfo;

class PSR0LocatorSpec extends ObjectBehavior
{
    private $srcPath;
    private $specPath;

    function let(Filesystem $fs)
    {
        $this->srcPath  = realpath(__DIR__.'/../../../../src');
        $this->specPath = realpath(__DIR__.'/../../../../');
    }

    function it_is_a_locator()
    {
        $this->shouldBeAnInstanceOf('PhpSpec\Locator\ResourceLocatorInterface');
    }

    function its_priority_is_zero()
    {
        $this->getPriority()->shouldReturn(0);
    }

    function it_generates_fullSrcPath_from_srcPath_plus_namespace()
    {
        $this->beConstructedWith('Cust\Ns', 'spec', dirname(__DIR__), __DIR__);

        $this->getFullSrcPath()->shouldReturn(
            dirname(__DIR__).DIRECTORY_SEPARATOR.'Cust'.DIRECTORY_SEPARATOR.'Ns'.DIRECTORY_SEPARATOR
        );
    }

    function it_generates_fullSrcPath_from_srcPath_plus_namespace_cutting_psr4_prefix()
    {
        $this->beConstructedWith('psr4\prefix\Cust\Ns', 'spec', dirname(__DIR__), __DIR__, null, 'psr4\prefix');

        $this->getFullSrcPath()->shouldReturn(
            dirname(__DIR__).DIRECTORY_SEPARATOR.'Cust'.DIRECTORY_SEPARATOR.'Ns'.DIRECTORY_SEPARATOR
        );
    }

    function it_generates_proper_fullSrcPath_even_from_empty_namespace()
    {
        $this->beConstructedWith('', 'spec', dirname(__DIR__), __DIR__);

        $this->getFullSrcPath()->shouldReturn(dirname(__DIR__).DIRECTORY_SEPARATOR);
    }

    function it_generates_fullSpecPath_from_specPath_plus_namespace()
    {
        $this->beConstructedWith('C\N', 'spec', dirname(__DIR__), __DIR__);

        $this->getFullSpecPath()->shouldReturn(
            __DIR__.DIRECTORY_SEPARATOR.'spec'.DIRECTORY_SEPARATOR.'C'.DIRECTORY_SEPARATOR.'N'.DIRECTORY_SEPARATOR
        );
    }

    function it_generates_fullSpecPath_from_specPath_plus_namespace_cutting_psr4_prefix()
    {
        $this->beConstructedWith('p\pf\C\N', 'spec', dirname(__DIR__), __DIR__, null, 'p\pf');

        $this->getFullSpecPath()->shouldReturn(
            __DIR__.DIRECTORY_SEPARATOR.'spec'.DIRECTORY_SEPARATOR.'C'.DIRECTORY_SEPARATOR.'N'.DIRECTORY_SEPARATOR
        );
    }

    function it_generates_proper_fullSpecPath_even_from_empty_src_namespace()
    {
        $this->beConstructedWith('', 'spec', dirname(__DIR__), __DIR__);

        $this->getFullSpecPath()->shouldReturn(
            __DIR__.DIRECTORY_SEPARATOR.'spec'.DIRECTORY_SEPARATOR
        );
    }

    function it_stores_srcNamespace_it_was_constructed_with()
    {
        $this->beConstructedWith('Some\Namespace', 'spec', dirname(__DIR__), __DIR__);

        $this->getSrcNamespace()->shouldReturn('Some\Namespace\\');
    }

    function it_trims_srcNamespace_during_construction()
    {
        $this->beConstructedWith('\\Some\Namespace\\', 'spec', dirname(__DIR__), __DIR__);

        $this->getSrcNamespace()->shouldReturn('Some\Namespace\\');
    }

    function it_supports_empty_namespace_argument()
    {
        $this->beConstructedWith('', 'spec', dirname(__DIR__), __DIR__);

        $this->getSrcNamespace()->shouldReturn('');
    }

    function it_generates_specNamespace_using_srcNamespace_and_specPrefix()
    {
        $this->beConstructedWith('Some\Namespace', 'spec', dirname(__DIR__), __DIR__);

        $this->getSpecNamespace()->shouldReturn('spec\Some\Namespace\\');
    }

    function it_trims_specNamespace_during_construction()
    {
        $this->beConstructedWith('\\Some\Namespace\\', '\\spec\\ns\\', dirname(__DIR__), __DIR__);

        $this->getSpecNamespace()->shouldReturn('spec\ns\Some\Namespace\\');
    }

    function it_generates_proper_specNamespace_for_empty_srcNamespace()
    {
        $this->beConstructedWith('', 'spec', dirname(__DIR__), __DIR__);

        $this->getSpecNamespace()->shouldReturn('spec\\');
    }

    function it_finds_all_resources_from_tracked_specPath(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('', 'spec', dirname(__DIR__), __DIR__, $fs);
        $path     = __DIR__.DIRECTORY_SEPARATOR.'spec'.DIRECTORY_SEPARATOR;
        $filePath = __DIR__.$this->convert_to_path('/spec/Some/ClassSpec.php');

        $fs->pathExists($path)->willReturn(true);
        $fs->findSpecFilesIn($path)->willReturn(array($file));
        $fs->getFileContents($filePath)->willReturn('<?php namespace spec\\Some; class ClassSpec {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $resources = $this->getAllResources();
        $resources->shouldHaveCount(1);
        $resources[0]->getSpecClassname()->shouldReturn('spec\Some\ClassSpec');
    }

    function it_returns_empty_array_if_tracked_specPath_does_not_exist(Filesystem $fs)
    {
        $this->beConstructedWith('', 'spec', dirname(__DIR__), __DIR__, $fs);
        $path = __DIR__.DIRECTORY_SEPARATOR.'spec'.DIRECTORY_SEPARATOR;

        $fs->pathExists($path)->willReturn(false);

        $resources = $this->getAllResources();
        $resources->shouldHaveCount(0);
    }

    function it_supports_folder_queries_in_srcPath()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsQuery($this->srcPath.'/PhpSpec')->shouldReturn(true);
    }

    function it_supports_srcPath_queries()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsQuery($this->srcPath)->shouldReturn(true);
    }

    function it_supports_file_queries_in_srcPath()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsQuery(
            realpath($this->srcPath.'/PhpSpec/ServiceContainer.php')
        )->shouldReturn(true);
    }

    function it_supports_folder_queries_in_specPath()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsQuery($this->specPath.'/spec/PhpSpec')->shouldReturn(true);
    }

    function it_supports_specPath_queries()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsQuery($this->specPath.'/spec')->shouldReturn(true);
    }

    function it_supports_file_queries_in_specPath()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsQuery(
            realpath($this->specPath.'/spec/PhpSpec/ServiceContainerSpec.php')
        )->shouldReturn(true);
    }

    function it_does_not_support_any_other_queries()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsQuery('/')->shouldReturn(false);
    }

    function it_finds_spec_resources_via_srcPath(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/ContainerSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(true);
        $fs->findSpecFilesIn($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(array($file));
        $fs->getFileContents($filePath)->willReturn('<?php namespace spec\\PhpSpec; class Container {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $resources = $this->findResources($this->srcPath);
        $resources->shouldHaveCount(1);
        $resources[0]->getSrcClassname()->shouldReturn('PhpSpec\Container');
    }

    function it_finds_spec_resources_with_classname_underscores_via_srcPath(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/Some/ClassSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(true);
        $fs->findSpecFilesIn($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(array($file));
        $fs->getFileContents($filePath)->willReturn('<?php namespace spec\\PhpSpec; class Some_Class {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $resources = $this->findResources($this->srcPath);
        $resources->shouldHaveCount(1);
        $resources[0]->getSrcClassname()->shouldReturn('PhpSpec\Some_Class');
    }

    function it_finds_spec_resources_via_fullSrcPath(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/Console/AppSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/Console/'))->willReturn(true);
        $fs->findSpecFilesIn($this->specPath.$this->convert_to_path('/spec/PhpSpec/Console/'))->willReturn(array($file));
        $fs->getFileContents($filePath)->willReturn('<?php namespace spec\\PhpSpec\\Console; class App {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $resources = $this->findResources($this->srcPath.$this->convert_to_path('/PhpSpec/Console'));
        $resources->shouldHaveCount(1);
        $resources[0]->getSrcClassname()->shouldReturn('PhpSpec\Console\App');
    }

    function it_finds_spec_resources_via_specPath(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/Runner/ExampleRunnerSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/Runner/'))->willReturn(true);
        $fs->findSpecFilesIn($this->specPath.$this->convert_to_path('/spec/PhpSpec/Runner/'))->willReturn(array($file));
        $fs->getFileContents($filePath)->willReturn('<?php namespace spec\\PhpSpec\\Runner; class ExampleRunner {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $resources = $this->findResources($this->specPath.$this->convert_to_path('/spec/PhpSpec/Runner'));
        $resources->shouldHaveCount(1);
        $resources[0]->getSrcClassname()->shouldReturn('PhpSpec\Runner\ExampleRunner');
    }

    function it_finds_single_spec_via_srcPath(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/ServiceContainerSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/ServiceContainerSpec.php'))->willReturn(true);
        $fs->getFileContents($filePath)->willReturn('<?php namespace spec\\PhpSpec; class ServiceContainer {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $resources = $this->findResources($this->srcPath.$this->convert_to_path('/PhpSpec/ServiceContainer.php'));
        $resources->shouldHaveCount(1);
        $resources[0]->getSrcClassname()->shouldReturn('PhpSpec\ServiceContainer');
    }

    function it_finds_single_spec_via_specPath(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/ServiceContainerSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/ServiceContainerSpec.php'))->willReturn(true);
        $fs->getFileContents($filePath)->willReturn('<?php namespace spec\\PhpSpec; class ServiceContainer {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $resources = $this->findResources($filePath);
        $resources->shouldHaveCount(1);
        $resources[0]->getSrcClassname()->shouldReturn('PhpSpec\ServiceContainer');
    }

    function it_returns_empty_array_if_nothing_found(Filesystem $fs)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $fs->pathExists($this->specPath.'/spec/PhpSpec/App/')->willReturn(false);

        $resources = $this->findResources($this->srcPath.'/PhpSpec/App');
        $resources->shouldHaveCount(0);
    }

    function it_throws_an_exception_on_no_class_definition(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/Some/ClassSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(true);
        $fs->findSpecFilesIn($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(array($file));
        $fs->getFileContents($filePath)->willReturn('no class definition');
        $file->getRealPath()->willReturn($filePath);

        $exception = new \RuntimeException('Spec file does not contains any class definition.');

        $this->shouldThrow($exception)->duringFindResources($this->srcPath);
    }

    function it_does_not_throw_an_exception_on_no_class_definition_if_file_not_suffixed_with_spec(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/Some/Class.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(true);
        $fs->findSpecFilesIn($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(array());
        $fs->getFileContents($filePath)->willReturn('no class definition');
        $file->getRealPath()->willReturn($filePath);

        $exception = new \RuntimeException('Spec file does not contains any class definition.');

        $this->shouldNotThrow($exception)->duringFindResources($this->srcPath);
    }

    function it_throws_an_exception_when_spec_class_not_in_the_base_specs_namespace(Filesystem $fs, SplFileInfo $file)
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath, $fs);

        $filePath = $this->specPath.$this->convert_to_path('/spec/PhpSpec/Some/ClassSpec.php');

        $fs->pathExists($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(true);
        $fs->findSpecFilesIn($this->specPath.$this->convert_to_path('/spec/PhpSpec/'))->willReturn(array($file));
        $fs->getFileContents($filePath)->willReturn('<?php namespace InvalidSpecNamespace\\PhpSpec; class ServiceContainer {} ?>');
        $file->getRealPath()->willReturn($filePath);

        $exception = new \RuntimeException('Spec class must be in the base spec namespace `spec\\PhpSpec\\`.');

        $this->shouldThrow($exception)->duringFindResources($this->srcPath);
    }

    function it_supports_classes_from_srcNamespace()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsClass('PhpSpec\ServiceContainer')->shouldReturn(true);
    }

    function it_supports_backslashed_classes_from_srcNamespace()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsClass('PhpSpec/ServiceContainer')->shouldReturn(true);
    }

    function it_supports_classes_from_specNamespace()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsClass('spec\PhpSpec\ServiceContainer')->shouldReturn(true);
    }

    function it_supports_backslashed_classes_from_specNamespace()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsClass('spec/PhpSpec/ServiceContainer')->shouldReturn(true);
    }

    function it_supports_any_class_if_srcNamespace_is_empty()
    {
        $this->beConstructedWith('', 'spec', $this->srcPath, $this->specPath);

        $this->supportsClass('ServiceContainer')->shouldReturn(true);
    }

    function it_does_not_support_anything_else()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $this->supportsClass('Acme\Any')->shouldReturn(false);
    }

    function it_creates_resource_from_src_class()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $resource = $this->createResource('PhpSpec\Console\Application');

        $resource->getSrcClassname()->shouldReturn('PhpSpec\Console\Application');
        $resource->getSpecClassname()->shouldReturn('spec\PhpSpec\Console\ApplicationSpec');
    }

    function it_creates_resource_from_backslashed_src_class()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $resource = $this->createResource('PhpSpec/Console/Application');

        $resource->getSrcClassname()->shouldReturn('PhpSpec\Console\Application');
        $resource->getSpecClassname()->shouldReturn('spec\PhpSpec\Console\ApplicationSpec');
    }

    function it_creates_resource_from_spec_class()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $resource = $this->createResource('spec\PhpSpec\Console\Application');

        $resource->getSrcClassname()->shouldReturn('PhpSpec\Console\Application');
        $resource->getSpecClassname()->shouldReturn('spec\PhpSpec\Console\ApplicationSpec');
    }

    function it_creates_resource_from_backslashed_spec_class()
    {
        $this->beConstructedWith('PhpSpec', 'spec', $this->srcPath, $this->specPath);

        $resource = $this->createResource('spec/PhpSpec/Console/Application');

        $resource->getSrcClassname()->shouldReturn('PhpSpec\Console\Application');
        $resource->getSpecClassname()->shouldReturn('spec\PhpSpec\Console\ApplicationSpec');
    }

    function it_creates_resource_from_src_class_even_if_srcNamespace_is_empty()
    {
        $this->beConstructedWith('', 'spec', $this->srcPath, $this->specPath);

        $resource = $this->createResource('Console\Application');

        $resource->getSrcClassname()->shouldReturn('Console\Application');
        $resource->getSpecClassname()->shouldReturn('spec\Console\ApplicationSpec');
    }

    function it_throws_an_exception_on_non_PSR0_resource()
    {
        $this->beConstructedWith('', 'spec', $this->srcPath, $this->specPath);

        $exception = new \InvalidArgumentException(
            'String "Non-PSR0/Namespace" is not a valid class name.'.PHP_EOL.
            'Please see reference document: https://github.com/php-fig/fig-standards/blob/master/accepted/PSR-0.md'
        );

        $this->shouldThrow($exception)->duringCreateResource('Non-PSR0/Namespace');
    }

    function it_throws_an_exception_on_PSR0_resource_with_double_backslash()
    {
        $this->beConstructedWith('', 'spec', $this->srcPath, $this->specPath);

        $exception = new \InvalidArgumentException(
            'String "NonPSR0\\\\Namespace" is not a valid class name.'.PHP_EOL.
            'Please see reference document: https://github.com/php-fig/fig-standards/blob/master/accepted/PSR-0.md'
        );

        $this->shouldThrow($exception)->duringCreateResource('NonPSR0\\\\Namespace');
    }

    function it_throws_an_exception_on_PSR0_resource_with_slash_on_the_end()
    {
        $this->beConstructedWith('', 'spec', $this->srcPath, $this->specPath);

        $exception = new \InvalidArgumentException(
            'String "Namespace/" is not a valid class name.'.PHP_EOL.
            'Please see reference document: https://github.com/php-fig/fig-standards/blob/master/accepted/PSR-0.md'
        );

        $this->shouldThrow($exception)->duringCreateResource('Namespace/');
    }

    function it_throws_an_exception_on_PSR4_prefix_not_matching_namespace()
    {
        $exception = new \InvalidArgumentException(
            'PSR4 prefix doesn\'t match given class namespace.'.PHP_EOL
        );

        $this->shouldThrow($exception)->during('__construct', array('p\pf\N\S', 'spec', $this->srcPath, $this->specPath, null, 'wrong\prefix'));
    }

    private function convert_to_path($path)
    {
        if ('/' === DIRECTORY_SEPARATOR) {
            return $path;
        }

        return str_replace('/', DIRECTORY_SEPARATOR, $path);
    }
}
