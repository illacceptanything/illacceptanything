<?php namespace Illuminate\Routing;

use Illuminate\Support\Str;
use Illuminate\Http\Response;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Traits\Macroable;
use Illuminate\Contracts\Support\Arrayable;
use Illuminate\Contracts\View\Factory as ViewFactory;
use Symfony\Component\HttpFoundation\StreamedResponse;
use Symfony\Component\HttpFoundation\BinaryFileResponse;
use Illuminate\Contracts\Routing\ResponseFactory as FactoryContract;

class ResponseFactory implements FactoryContract {

	use Macroable;

	/**
	 * The view factory implementation.
	 *
	 * @var \Illuminate\Contracts\View\Factory
	 */
	protected $view;

	/**
	 * The redirector instance.
	 *
	 * @var \Illuminate\Routing\Redirector
	 */
	protected $redirector;

	/**
	 * Create a new response factory instance.
	 *
	 * @param  \Illuminate\Contracts\View\Factory  $view
	 * @param  \Illuminate\Routing\Redirector  $redirector
	 * @return void
	 */
	public function __construct(ViewFactory $view, Redirector $redirector)
	{
		$this->view = $view;
		$this->redirector = $redirector;
	}

	/**
	 * Return a new response from the application.
	 *
	 * @param  string  $content
	 * @param  int     $status
	 * @param  array   $headers
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function make($content = '', $status = 200, array $headers = array())
	{
		return new Response($content, $status, $headers);
	}

	/**
	 * Return a new view response from the application.
	 *
	 * @param  string  $view
	 * @param  array   $data
	 * @param  int     $status
	 * @param  array   $headers
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function view($view, $data = array(), $status = 200, array $headers = array())
	{
		return static::make($this->view->make($view, $data), $status, $headers);
	}

	/**
	 * Return a new JSON response from the application.
	 *
	 * @param  string|array  $data
	 * @param  int    $status
	 * @param  array  $headers
	 * @param  int    $options
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function json($data = array(), $status = 200, array $headers = array(), $options = 0)
	{
		if ($data instanceof Arrayable)
		{
			$data = $data->toArray();
		}

		return new JsonResponse($data, $status, $headers, $options);
	}

	/**
	 * Return a new JSONP response from the application.
	 *
	 * @param  string  $callback
	 * @param  string|array  $data
	 * @param  int    $status
	 * @param  array  $headers
	 * @param  int    $options
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function jsonp($callback, $data = array(), $status = 200, array $headers = array(), $options = 0)
	{
		return $this->json($data, $status, $headers, $options)->setCallback($callback);
	}

	/**
	 * Return a new streamed response from the application.
	 *
	 * @param  \Closure  $callback
	 * @param  int      $status
	 * @param  array    $headers
	 * @return \Symfony\Component\HttpFoundation\StreamedResponse
	 */
	public function stream($callback, $status = 200, array $headers = array())
	{
		return new StreamedResponse($callback, $status, $headers);
	}

	/**
	 * Create a new file download response.
	 *
	 * @param  \SplFileInfo|string  $file
	 * @param  string  $name
	 * @param  array   $headers
	 * @param  null|string  $disposition
	 * @return \Symfony\Component\HttpFoundation\BinaryFileResponse
	 */
	public function download($file, $name = null, array $headers = array(), $disposition = 'attachment')
	{
		$response = new BinaryFileResponse($file, 200, $headers, true, $disposition);

		if ( ! is_null($name))
		{
			return $response->setContentDisposition($disposition, $name, str_replace('%', '', Str::ascii($name)));
		}

		return $response;
	}

	/**
	 * Create a new redirect response to the given path.
	 *
	 * @param  string  $path
	 * @param  int     $status
	 * @param  array   $headers
	 * @param  bool    $secure
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function redirectTo($path, $status = 302, $headers = array(), $secure = null)
	{
		return $this->redirector->to($path, $status, $headers, $secure);
	}

	/**
	 * Create a new redirect response to a named route.
	 *
	 * @param  string  $route
	 * @param  array   $parameters
	 * @param  int     $status
	 * @param  array   $headers
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function redirectToRoute($route, $parameters = array(), $status = 302, $headers = array())
	{
		return $this->redirector->route($route, $parameters, $status, $headers);
	}

	/**
	 * Create a new redirect response to a controller action.
	 *
	 * @param  string  $action
	 * @param  array   $parameters
	 * @param  int     $status
	 * @param  array   $headers
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function redirectToAction($action, $parameters = array(), $status = 302, $headers = array())
	{
		return $this->redirector->action($action, $parameters, $status, $headers);
	}

	/**
	 * Create a new redirect response, while putting the current URL in the session.
	 *
	 * @param  string  $path
	 * @param  int     $status
	 * @param  array   $headers
	 * @param  bool    $secure
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function redirectGuest($path, $status = 302, $headers = array(), $secure = null)
	{
		return $this->redirector->guest($path, $status, $headers, $secure);
	}

	/**
	 * Create a new redirect response to the previously intended location.
	 *
	 * @param  string  $default
	 * @param  int     $status
	 * @param  array   $headers
	 * @param  bool    $secure
	 * @return \Symfony\Component\HttpFoundation\Response
	 */
	public function redirectToIntended($default = '/', $status = 302, $headers = array(), $secure = null)
	{
		return $this->redirector->intended($default, $status, $headers, $secure);
	}

}
