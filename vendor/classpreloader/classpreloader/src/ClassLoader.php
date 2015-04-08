<?php

namespace ClassPreloader;

require_once __DIR__ . '/ClassNode.php';
require_once __DIR__ . '/ClassList.php';

/**
 * This is the class loader class.
 *
 * This creates an autoloader that intercepts and keeps track of each include
 * in order that files must be included. This autoloader proxies to all other
 * underlying autoloaders.
 */
class ClassLoader
{
    /**
     * The list of loaded classes.
     *
     * @var \ClassPreloader\ClassList
     */
    public $classList;

    /**
     * Create a new class loader.
     *
     * @return void
     */
    public function __construct()
    {
        $this->classList = new ClassList();
    }

    /**
     * Destroy the class loader.
     *
     * This makes sure we're unregistered from the autoloader.
     *
     * @return void
     */
    public function __destruct()
    {
        $this->unregister();
    }

    /**
     * Wrap a block of code in the autoloader and get a list of loaded classes.
     *
     * @param callable $func
     *
     * @return \ClassPreloader\Config
     */
    public static function getIncludes($func)
    {
        $loader = new static();
        call_user_func($func, $loader);
        $loader->unregister();

        $config = new Config();
        foreach ($loader->getFilenames() as $file) {
            $config->addFile($file);
        }

        return $config;
    }

    /**
     * Registers this instance as an autoloader.
     *
     * @return void
     */
    public function register()
    {
        spl_autoload_register(array($this, 'loadClass'), true, true);
    }

    /**
     * Unregisters this instance as an autoloader.
     *
     * @return void
     */
    public function unregister()
    {
        spl_autoload_unregister(array($this, 'loadClass'));
    }

    /**
     * Loads the given class, interface or trait.
     *
     * We'll return true if it was loaded.
     *
     * @param string $class
     *
     * @return bool|null
     */
    public function loadClass($class)
    {
        foreach (spl_autoload_functions() as $func) {
            if (is_array($func) && $func[0] === $this) {
                continue;
            }
            $this->classList->push($class);
            if (call_user_func($func, $class)) {
                break;
            }
        }

        $this->classList->next();

        return true;
    }

    /**
     * Get an array of loaded file names in order of loading.
     *
     * @return array
     */
    public function getFilenames()
    {
        $files = array();
        foreach ($this->classList->getClasses() as $class) {
            // Push interfaces before classes if not already loaded
            try {
                $r = new \ReflectionClass($class);
                foreach ($r->getInterfaces() as $inf) {
                    $name = $inf->getFileName();
                    if ($name && !in_array($name, $files)) {
                        $files[] = $name;
                    }
                }
                if (!in_array($r->getFileName(), $files)) {
                    $files[] = $r->getFileName();
                }
            } catch (\ReflectionException $e) {
                // We ignore all exceptions related to reflection,
                // because in some cases class can't exists. This
                // can be if you use in your code constuctions like
                //
                // if (class_exists('SomeClass')) { // <-- here will trigger autoload
                //      class SomeSuperClass extends SomeClass {
                //      }
                // }
                //
                // We ignore all problems with classes, interfaces and
                // traits.
            }
        }

        return $files;
    }
}
