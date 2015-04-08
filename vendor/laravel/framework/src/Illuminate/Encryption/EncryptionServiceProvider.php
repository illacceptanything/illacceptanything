<?php namespace Illuminate\Encryption;

use Illuminate\Support\ServiceProvider;

class EncryptionServiceProvider extends ServiceProvider {

	/**
	 * Register the service provider.
	 *
	 * @return void
	 */
	public function register()
	{
		$this->app->singleton('encrypter', function($app)
		{
			$encrypter =  new Encrypter($app['config']['app.key']);

			if ($app['config']->has('app.cipher'))
			{
				$encrypter->setCipher($app['config']['app.cipher']);
			}

			return $encrypter;
		});
	}

}
