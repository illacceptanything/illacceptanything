<?php namespace Illuminate\Session;

use Illuminate\Support\ServiceProvider;

class CommandsServiceProvider extends ServiceProvider {

	/**
	 * Indicates if loading of the provider is deferred.
	 *
	 * @var bool
	 */
	protected $defer = true;

	/**
	 * Register the service provider.
	 *
	 * @return void
	 */
	public function register()
	{
		$this->app->singleton('command.session.database', function($app)
		{
			return new Console\SessionTableCommand($app['files'], $app['composer']);
		});

		$this->commands('command.session.database');
	}

	/**
	 * Get the services provided by the provider.
	 *
	 * @return array
	 */
	public function provides()
	{
		return array('command.session.database');
	}

}
