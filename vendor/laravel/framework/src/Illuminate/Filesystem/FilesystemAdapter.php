<?php namespace Illuminate\Filesystem;

use InvalidArgumentException;
use Illuminate\Support\Collection;
use League\Flysystem\AdapterInterface;
use League\Flysystem\FilesystemInterface;
use League\Flysystem\FileNotFoundException;
use Illuminate\Contracts\Filesystem\Filesystem as FilesystemContract;
use Illuminate\Contracts\Filesystem\Cloud as CloudFilesystemContract;
use Illuminate\Contracts\Filesystem\FileNotFoundException as ContractFileNotFoundException;

class FilesystemAdapter implements FilesystemContract, CloudFilesystemContract {

	/**
	 * The Flysystem filesystem implementation.
	 *
	 * @var \League\Flysystem\FilesystemInterface
	 */
	protected $driver;

	/**
	 * Create a new filesystem adapter instance.
	 *
	 * @param  \League\Flysystem\FilesystemInterface  $driver
	 * @return void
	 */
	public function __construct(FilesystemInterface $driver)
	{
		$this->driver = $driver;
	}

	/**
	 * Determine if a file exists.
	 *
	 * @param  string  $path
	 * @return bool
	 */
	public function exists($path)
	{
		return $this->driver->has($path);
	}

	/**
	 * Get the contents of a file.
	 *
	 * @param  string  $path
	 * @return string
	 *
	 * @throws \Illuminate\Contracts\Filesystem\FileNotFoundException
	 */
	public function get($path)
	{
		try
		{
			return $this->driver->read($path);
		}
		catch (FileNotFoundException $e)
		{
			throw new ContractFileNotFoundException($path, $e->getCode(), $e);
		}
	}

	/**
	 * Write the contents of a file.
	 *
	 * @param  string  $path
	 * @param  string  $contents
	 * @param  string  $visibility
	 * @return bool
	 */
	public function put($path, $contents, $visibility = null)
	{
		return $this->driver->put($path, $contents, ['visibility' => $this->parseVisibility($visibility)]);
	}

	/**
	 * Get the visibility for the given path.
	 *
	 * @param  string  $path
	 * @return string
	 */
	public function getVisibility($path)
	{
		if ($this->driver->getVisibility($path) == AdapterInterface::VISIBILITY_PUBLIC)
		{
			return FilesystemContract::VISIBILITY_PUBLIC;
		}

		return FilesystemContract::VISIBILITY_PRIVATE;
	}

	/**
	 * Set the visibility for the given path.
	 *
	 * @param  string  $path
	 * @param  string  $visibility
	 * @return void
	 */
	public function setVisibility($path, $visibility)
	{
		return $this->driver->setVisibility($path, $this->parseVisibility($visibility));
	}

	/**
	 * Prepend to a file.
	 *
	 * @param  string  $path
	 * @param  string  $data
	 * @return int
	 */
	public function prepend($path, $data)
	{
		return $this->put($path, $data.PHP_EOL.$this->get($path));
	}

	/**
	 * Append to a file.
	 *
	 * @param  string  $path
	 * @param  string  $data
	 * @return int
	 */
	public function append($path, $data)
	{
		return $this->put($path, $this->get($path).PHP_EOL.$data);
	}

	/**
	 * Delete the file at a given path.
	 *
	 * @param  string|array  $paths
	 * @return bool
	 */
	public function delete($paths)
	{
		$paths = is_array($paths) ? $paths : func_get_args();

		foreach ($paths as $path)
		{
			$this->driver->delete($path);
		}

		return true;
	}

	/**
	 * Copy a file to a new location.
	 *
	 * @param  string  $from
	 * @param  string  $to
	 * @return bool
	 */
	public function copy($from, $to)
	{
		return $this->driver->copy($from, $to);
	}

	/**
	 * Move a file to a new location.
	 *
	 * @param  string  $from
	 * @param  string  $to
	 * @return bool
	 */
	public function move($from, $to)
	{
		$this->driver->copy($from, $to);

		$this->driver->delete($from);
	}

	/**
	 * Get the file size of a given file.
	 *
	 * @param  string  $path
	 * @return int
	 */
	public function size($path)
	{
		return $this->driver->getSize($path);
	}

	/**
	 * Get the mime-type of a given file.
	 *
	 * @param  string  $path
	 * @return string|false
	 */
	public function mimeType($path)
	{
		return $this->driver->getMimetype($path);
	}

	/**
	 * Get the file's last modification time.
	 *
	 * @param  string  $path
	 * @return int
	 */
	public function lastModified($path)
	{
		return $this->driver->getTimestamp($path);
	}

	/**
	 * Get an array of all files in a directory.
	 *
	 * @param  string|null  $directory
	 * @param  bool  $recursive
	 * @return array
	 */
	public function files($directory = null, $recursive = false)
	{
		$contents = $this->driver->listContents($directory, $recursive);

		return $this->filterContentsByType($contents, 'file');
	}

	/**
	 * Get all of the files from the given directory (recursive).
	 *
	 * @param  string|null  $directory
	 * @return array
	 */
	public function allFiles($directory = null)
	{
		return $this->files($directory, true);
	}

	/**
	 * Get all of the directories within a given directory.
	 *
	 * @param  string|null  $directory
	 * @param  bool  $recursive
	 * @return array
	 */
	public function directories($directory = null, $recursive = false)
	{
		$contents = $this->driver->listContents($directory, $recursive);

		return $this->filterContentsByType($contents, 'dir');
	}

	/**
	 * Get all (recursive) of the directories within a given directory.
	 *
	 * @param  string|null  $directory
	 * @param  bool  $recursive
	 * @return array
	 */
	public function allDirectories($directory = null, $recursive = false)
	{
		return $this->directories($directory, true);
	}

	/**
	 * Create a directory.
	 *
	 * @param  string  $path
	 * @return bool
	 */
	public function makeDirectory($path)
	{
		return $this->driver->createDir($path);
	}

	/**
	 * Recursively delete a directory.
	 *
	 * @param  string  $directory
	 * @return bool
	 */
	public function deleteDirectory($directory)
	{
		return $this->driver->deleteDir($directory);
	}

	/**
	 * Get the Flysystem driver.
	 *
	 * @return \League\Flysystem\FilesystemInterface
	 */
	public function getDriver()
	{
		return $this->driver;
	}

	/**
	 * Filter directory contents by type.
	 *
	 * @param  array  $contents
	 * @param  string  $type
	 * @return array
	 */
	protected function filterContentsByType($contents, $type)
	{
		return Collection::make($contents)
			->where('type', $type)
			->fetch('path')
			->values()->all();
	}

	/**
	 * Parse the given visibility value.
	 *
	 * @param  string|null  $visibility
	 * @return string
	 * @throws \InvalidArgumentException
	 */
	protected function parseVisibility($visibility)
	{
		if (is_null($visibility)) return;

		switch ($visibility)
		{
			case FilesystemContract::VISIBILITY_PUBLIC:
				return AdapterInterface::VISIBILITY_PUBLIC;

			case FilesystemContract::VISIBILITY_PRIVATE:
				return AdapterInterface::VISIBILITY_PRIVATE;
		}

		throw new InvalidArgumentException('Unknown visibility: '.$visibility);
	}

}
