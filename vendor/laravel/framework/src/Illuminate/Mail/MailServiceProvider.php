<?php namespace Illuminate\Mail;

use Swift_Mailer;
use Illuminate\Support\ServiceProvider;

class MailServiceProvider extends ServiceProvider {

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
		$this->app->singleton('mailer', function($app)
		{
			$this->registerSwiftMailer();

			// Once we have create the mailer instance, we will set a container instance
			// on the mailer. This allows us to resolve mailer classes via containers
			// for maximum testability on said classes instead of passing Closures.
			$mailer = new Mailer(
				$app['view'], $app['swift.mailer'], $app['events']
			);

			$this->setMailerDependencies($mailer, $app);

			// If a "from" address is set, we will set it on the mailer so that all mail
			// messages sent by the applications will utilize the same "from" address
			// on each one, which makes the developer's life a lot more convenient.
			$from = $app['config']['mail.from'];

			if (is_array($from) && isset($from['address']))
			{
				$mailer->alwaysFrom($from['address'], $from['name']);
			}

			// Here we will determine if the mailer should be in "pretend" mode for this
			// environment, which will simply write out e-mail to the logs instead of
			// sending it over the web, which is useful for local dev environments.
			$pretend = $app['config']->get('mail.pretend', false);

			$mailer->pretend($pretend);

			return $mailer;
		});
	}

	/**
	 * Set a few dependencies on the mailer instance.
	 *
	 * @param  \Illuminate\Mail\Mailer  $mailer
	 * @param  \Illuminate\Foundation\Application  $app
	 * @return void
	 */
	protected function setMailerDependencies($mailer, $app)
	{
		$mailer->setContainer($app);

		if ($app->bound('log'))
		{
			$mailer->setLogger($app['log']->getMonolog());
		}

		if ($app->bound('queue'))
		{
			$mailer->setQueue($app['queue.connection']);
		}
	}

	/**
	 * Register the Swift Mailer instance.
	 *
	 * @return void
	 */
	public function registerSwiftMailer()
	{
		$this->registerSwiftTransport();

		// Once we have the transporter registered, we will register the actual Swift
		// mailer instance, passing in the transport instances, which allows us to
		// override this transporter instances during app start-up if necessary.
		$this->app['swift.mailer'] = $this->app->share(function($app)
		{
			return new Swift_Mailer($app['swift.transport']->driver());
		});
	}

	/**
	 * Register the Swift Transport instance.
	 *
	 * @return void
	 */
	protected function registerSwiftTransport()
	{
		$this->app['swift.transport'] = $this->app->share(function($app)
		{
			return new TransportManager($app);
		});
	}

	/**
	 * Get the services provided by the provider.
	 *
	 * @return array
	 */
	public function provides()
	{
		return ['mailer', 'swift.mailer', 'swift.transport'];
	}

}
