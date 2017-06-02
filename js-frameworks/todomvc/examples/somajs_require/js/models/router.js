/*global define:false */
(function () {

	'use strict';

	define(['director'], function (Router) {

		var RouterModel = function (dispatcher) {

			// create the router (director.js)
			var router = new Router().init().configure({
				notfound: render
			});

			// dispatch a custom event to render the template on a route change
			router.on(/.*/, render);

			function render() {
				dispatcher.dispatch('render');
			}

			return {
				getRoute: function () {
					return router.getRoute()[0];
				}
			};
		};

		return RouterModel;

	});

})();
