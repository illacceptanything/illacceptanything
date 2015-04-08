<?php namespace Illuminate\View;

use Closure;
use ArrayAccess;
use BadMethodCallException;
use Illuminate\Support\MessageBag;
use Illuminate\Contracts\Support\Arrayable;
use Illuminate\View\Engines\EngineInterface;
use Illuminate\Contracts\Support\Renderable;
use Illuminate\Contracts\Support\MessageProvider;
use Illuminate\Contracts\View\View as ViewContract;

class View implements ArrayAccess, ViewContract {

	/**
	 * The view factory instance.
	 *
	 * @var \Illuminate\View\Factory
	 */
	protected $factory;

	/**
	 * The engine implementation.
	 *
	 * @var \Illuminate\View\Engines\EngineInterface
	 */
	protected $engine;

	/**
	 * The name of the view.
	 *
	 * @var string
	 */
	protected $view;

	/**
	 * The array of view data.
	 *
	 * @var array
	 */
	protected $data;

	/**
	 * The path to the view file.
	 *
	 * @var string
	 */
	protected $path;

	/**
	 * Create a new view instance.
	 *
	 * @param  \Illuminate\View\Factory  $factory
	 * @param  \Illuminate\View\Engines\EngineInterface  $engine
	 * @param  string  $view
	 * @param  string  $path
	 * @param  array   $data
	 * @return void
	 */
	public function __construct(Factory $factory, EngineInterface $engine, $view, $path, $data = array())
	{
		$this->view = $view;
		$this->path = $path;
		$this->engine = $engine;
		$this->factory = $factory;

		$this->data = $data instanceof Arrayable ? $data->toArray() : (array) $data;
	}

	/**
	 * Get the string contents of the view.
	 *
	 * @param  \Closure|null  $callback
	 * @return string
	 */
	public function render(Closure $callback = null)
	{
		$contents = $this->renderContents();

		$response = isset($callback) ? $callback($this, $contents) : null;

		// Once we have the contents of the view, we will flush the sections if we are
		// done rendering all views so that there is nothing left hanging over when
		// another view gets rendered in the future by the application developer.
		$this->factory->flushSectionsIfDoneRendering();

		return $response ?: $contents;
	}

	/**
	 * Get the contents of the view instance.
	 *
	 * @return string
	 */
	protected function renderContents()
	{
		// We will keep track of the amount of views being rendered so we can flush
		// the section after the complete rendering operation is done. This will
		// clear out the sections for any separate views that may be rendered.
		$this->factory->incrementRender();

		$this->factory->callComposer($this);

		$contents = $this->getContents();

		// Once we've finished rendering the view, we'll decrement the render count
		// so that each sections get flushed out next time a view is created and
		// no old sections are staying around in the memory of an environment.
		$this->factory->decrementRender();

		return $contents;
	}

	/**
	 * Get the sections of the rendered view.
	 *
	 * @return array
	 */
	public function renderSections()
	{
		$env = $this->factory;

		return $this->render(function($view) use ($env)
		{
			return $env->getSections();
		});
	}

	/**
	 * Get the evaluated contents of the view.
	 *
	 * @return string
	 */
	protected function getContents()
	{
		return $this->engine->get($this->path, $this->gatherData());
	}

	/**
	 * Get the data bound to the view instance.
	 *
	 * @return array
	 */
	protected function gatherData()
	{
		$data = array_merge($this->factory->getShared(), $this->data);

		foreach ($data as $key => $value)
		{
			if ($value instanceof Renderable)
			{
				$data[$key] = $value->render();
			}
		}

		return $data;
	}

	/**
	 * Add a piece of data to the view.
	 *
	 * @param  string|array  $key
	 * @param  mixed   $value
	 * @return $this
	 */
	public function with($key, $value = null)
	{
		if (is_array($key))
		{
			$this->data = array_merge($this->data, $key);
		}
		else
		{
			$this->data[$key] = $value;
		}

		return $this;
	}

	/**
	 * Add a view instance to the view data.
	 *
	 * @param  string  $key
	 * @param  string  $view
	 * @param  array   $data
	 * @return $this
	 */
	public function nest($key, $view, array $data = array())
	{
		return $this->with($key, $this->factory->make($view, $data));
	}

	/**
	 * Add validation errors to the view.
	 *
	 * @param  \Illuminate\Contracts\Support\MessageProvider|array  $provider
	 * @return $this
	 */
	public function withErrors($provider)
	{
		if ($provider instanceof MessageProvider)
		{
			$this->with('errors', $provider->getMessageBag());
		}
		else
		{
			$this->with('errors', new MessageBag((array) $provider));
		}

		return $this;
	}

	/**
	 * Get the view factory instance.
	 *
	 * @return \Illuminate\View\Factory
	 */
	public function getFactory()
	{
		return $this->factory;
	}

	/**
	 * Get the view's rendering engine.
	 *
	 * @return \Illuminate\View\Engines\EngineInterface
	 */
	public function getEngine()
	{
		return $this->engine;
	}

	/**
	 * Get the name of the view.
	 *
	 * @return string
	 */
	public function name()
	{
		return $this->getName();
	}

	/**
	 * Get the name of the view.
	 *
	 * @return string
	 */
	public function getName()
	{
		return $this->view;
	}

	/**
	 * Get the array of view data.
	 *
	 * @return array
	 */
	public function getData()
	{
		return $this->data;
	}

	/**
	 * Get the path to the view file.
	 *
	 * @return string
	 */
	public function getPath()
	{
		return $this->path;
	}

	/**
	 * Set the path to the view.
	 *
	 * @param  string  $path
	 * @return void
	 */
	public function setPath($path)
	{
		$this->path = $path;
	}

	/**
	 * Determine if a piece of data is bound.
	 *
	 * @param  string  $key
	 * @return bool
	 */
	public function offsetExists($key)
	{
		return array_key_exists($key, $this->data);
	}

	/**
	 * Get a piece of bound data to the view.
	 *
	 * @param  string  $key
	 * @return mixed
	 */
	public function offsetGet($key)
	{
		return $this->data[$key];
	}

	/**
	 * Set a piece of data on the view.
	 *
	 * @param  string  $key
	 * @param  mixed   $value
	 * @return void
	 */
	public function offsetSet($key, $value)
	{
		$this->with($key, $value);
	}

	/**
	 * Unset a piece of data from the view.
	 *
	 * @param  string  $key
	 * @return void
	 */
	public function offsetUnset($key)
	{
		unset($this->data[$key]);
	}

	/**
	 * Get a piece of data from the view.
	 *
	 * @param  string  $key
	 * @return mixed
	 */
	public function &__get($key)
	{
		return $this->data[$key];
	}

	/**
	 * Set a piece of data on the view.
	 *
	 * @param  string  $key
	 * @param  mixed   $value
	 * @return void
	 */
	public function __set($key, $value)
	{
		$this->with($key, $value);
	}

	/**
	 * Check if a piece of data is bound to the view.
	 *
	 * @param  string  $key
	 * @return bool
	 */
	public function __isset($key)
	{
		return isset($this->data[$key]);
	}

	/**
	 * Remove a piece of bound data from the view.
	 *
	 * @param  string  $key
	 * @return bool
	 */
	public function __unset($key)
	{
		unset($this->data[$key]);
	}

	/**
	 * Dynamically bind parameters to the view.
	 *
	 * @param  string  $method
	 * @param  array   $parameters
	 * @return \Illuminate\View\View
	 *
	 * @throws \BadMethodCallException
	 */
	public function __call($method, $parameters)
	{
		if (starts_with($method, 'with'))
		{
			return $this->with(snake_case(substr($method, 4)), $parameters[0]);
		}

		throw new BadMethodCallException("Method [$method] does not exist on view.");
	}

	/**
	 * Get the string contents of the view.
	 *
	 * @return string
	 */
	public function __toString()
	{
		return $this->render();
	}

}
