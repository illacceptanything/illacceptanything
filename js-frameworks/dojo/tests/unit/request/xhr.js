define([
	'intern!object',
	'intern/chai!assert',
	'../../../request/xhr',
	'../../../errors/RequestTimeoutError',
	'../../../errors/CancelError',
	'dojo/promise/all',
	'dojo/query',
	'require'
], function (registerSuite, assert, xhr, RequestTimeoutError, CancelError, all, query, require) {
	var hasFormData = 'FormData' in this && typeof FormData === 'function';
	var hasResponseType = typeof XMLHttpRequest !== 'undefined' &&
		typeof new XMLHttpRequest().responseType !== 'undefined';
	var formData;

	registerSuite({
		name: 'dojo/request/xhr',

		'.get': function () {
			var promise = xhr.get('/__services/request/xhr', {
				preventCache: true,
				handleAs: 'json'
			});

			assert.isFunction(promise.then);
			assert.isFunction(promise.cancel);
			assert.isObject(promise.response);
			assert.isFunction(promise.response.then);
			assert.isFunction(promise.response.cancel);

			return promise.response.then(function (response) {
				assert.strictEqual(response.data.method, 'GET');
				assert.strictEqual(response.xhr.readyState, 4);
				return response;
			});
		},

		'.get 404': function () {
			var def = this.async(),
				promise = xhr.get(require.toUrl('./xhr_blarg.html'), {
					preventCache: true
				});

			promise.response.then(
				def.reject,
				def.callback(function (error) {
					assert.strictEqual(error.response.status, 404);
				})
			);
		},

		'.get with query': function () {
			var def = this.async(),
				promise = xhr.get('/__services/request/xhr?color=blue', {
					query: {
						foo: [ 'bar', 'baz' ],
						thud: 'thonk',
						xyzzy: 3
					},
					handleAs: 'json'
				});

			promise.response.then(def.callback(function (response) {
				assert.strictEqual(response.data.method, 'GET');
				var query = response.data.query;
				assert.ok(query.color && query.foo && query.foo.length && query.thud && query.xyzzy);
				assert.strictEqual(query.color, 'blue');
				assert.strictEqual(query.foo.length, 2);
				assert.strictEqual(query.thud, 'thonk');
				assert.strictEqual(query.xyzzy, '3');
				assert.strictEqual(response.url, '/__services/request/xhr?color=blue&foo=bar&foo=baz&thud=thonk&xyzzy=3');
			}));
		},

		'.post': function () {
			var def = this.async(),
				promise = xhr.post('/__services/request/xhr', {
					data: { color: 'blue' },
					handleAs: 'json'
				});

			promise.response.then(
				def.callback(function (response) {
					assert.strictEqual(response.data.method, 'POST');
					var payload = response.data.payload;

					assert.ok(payload && payload.color);
					assert.strictEqual(payload.color, 'blue');
				}),
				def.reject
			);
		},

		'.post with query': function () {
			var def = this.async(),
				promise = xhr.post('/__services/request/xhr', {
					query: {
						foo: [ 'bar', 'baz' ],
						thud: 'thonk',
						xyzzy: 3
					},
					data: { color: 'blue' },
					handleAs: 'json'
				});

			promise.then(
				def.callback(function (data) {
					assert.strictEqual(data.method, 'POST');
					var query = data.query,
						payload = data.payload;

					assert.ok(query);
					assert.deepEqual(query.foo, [ 'bar', 'baz' ]);
					assert.strictEqual(query.thud, 'thonk');
					assert.strictEqual(query.xyzzy, '3');

					assert.ok(payload);
					assert.strictEqual(payload.color, 'blue');
				}),
				def.reject
			);
		},

		'.post string payload': function () {
			var def = this.async(),
				promise = xhr.post('/__services/request/xhr', {
					data: 'foo=bar&color=blue&height=average',
					handleAs: 'json'
				});

			promise.then(
				def.callback(function (data) {
					assert.strictEqual(data.method, 'POST');

					var payload = data.payload;

					assert.ok(payload);
					assert.strictEqual(payload.foo, 'bar');
					assert.strictEqual(payload.color, 'blue');
					assert.strictEqual(payload.height, 'average');
				}),
				def.reject
			);
		},

		'.put': function () {
			var def = this.async(),
				promise = xhr.put('/__services/request/xhr', {
					query: { foo: 'bar' },
					data: { color: 'blue' },
					handleAs: 'json'
				});

			promise.then(
				def.callback(function (data) {
					assert.strictEqual(data.method, 'PUT');

					assert.ok(data.payload);
					assert.strictEqual(data.payload.color, 'blue');

					assert.ok(data.query);
					assert.strictEqual(data.query.foo, 'bar');
				}),
				def.reject
			);
		},

		'.del': function () {
			var def = this.async(),
				promise = xhr.del('/__services/request/xhr', {
					query: { foo: 'bar' },
					handleAs: 'json'
				});

			promise.then(
				def.callback(function (data) {
					assert.strictEqual(data.method, 'DELETE');
					assert.strictEqual(data.query.foo, 'bar');
				}),
				def.reject
			);
		},

		'timeout': function () {
			var def = this.async(),
				promise = xhr.get('/__services/request/xhr', {
					query: {
						delay: '3000'
					},
					timeout: 1000
				});

			promise.then(
				def.reject,
				def.callback(function (error) {
					assert.instanceOf(error, RequestTimeoutError);
				})
			);
		},

		cancel: function () {
			var def = this.async(),
				promise = xhr.get('/__services/request/xhr', {
					query: {
						delay: '3000'
					}
				});

			promise.then(
				def.reject,
				def.callback(function (error) {
					assert.instanceOf(error, CancelError);
				})
			);
			promise.cancel();
		},

		sync: function () {
			var called = false;

			xhr.get('/__services/request/xhr', {
				sync: true
			}).then(function () {
				called = true;
			});

			assert.ok(called);
		},

		'cross-domain fails': function () {
			var def = this.async();

			xhr.get('http://dojotoolkit.org').response.then(
				def.reject,
				function () {
					def.resolve(true);
				}
			);
		},

		headers: function () {
			var def = this.async();

			xhr.get('/__services/request/xhr').response.then(
				def.callback(function (response) {
					assert.notEqual(response.getHeader('Content-Type'), null);
				}),
				def.reject
			);
		},

		'custom Content-Type': function () {
			var def = this.async(),
				expectedContentType = 'application/x-test-xhr';

			function post(headers) {
				return xhr.post('/__services/request/xhr', {
					query: {
						'header-test': 'true'
					},
					headers: headers,
					data: 'testing',
					handleAs: 'json'
				});
			}

			all({
				lowercase: post({
					'content-type': expectedContentType
				}),
				uppercase: post({
					'CONTENT-TYPE': expectedContentType
				})
			}).then(
				def.callback(function (results) {
					assert.match(
						results.lowercase.headers['content-type'],
						/^application\/x-test-xhr(?:;.*)?$/
					);
					assert.match(
						results.uppercase.headers['content-type'],
						/^application\/x-test-xhr(?:;.*)?$/
					);
				}),
				def.reject
			);
		},

		'queryable xml': function () {
			var def = this.async();

			xhr.get('/__services/request/xhr/xml', {
				handleAs: 'xml'
			}).then(
				def.callback(function (xmlDoc) {
					var results = query('bar', xmlDoc);

					assert.strictEqual(results.length, 2);
				}),
				def.reject
			);
		},

		'form data': {
			setup: function () {
				if(!hasFormData) { return; }

				formData = new FormData();
				formData.append('foo', 'bar');
				formData.append('baz', 'blah');
			},

			post: function () {
				if(!hasFormData) {
					this.skip('No FormData to test');
				}

				var def = this.async();

				xhr.post('/__services/request/xhr/multipart', {
					data: formData,
					handleAs: 'json'
				}).then(
					def.callback(function (data) {
						assert.deepEqual(data, { foo: 'bar', baz: 'blah' });
					}),
					def.reject
				);
			},

			teardown: function () {
				formData = null;
			}
		},

		'response type': {
			'Blob': function () {
				if (!hasResponseType) {
					this.skip('No responseType to test');
				}

				return xhr.get('/__services/request/xhr/responseTypeGif', {
					handleAs: 'blob'
				}).then(function (response) {
					assert.strictEqual(response.constructor, Blob);
				});
			},

			'Blob POST': function () {
				if (!hasResponseType) {
					this.skip('No responseType to test');
				}

				return xhr.post('/__services/request/xhr/responseTypeGif', {
					handleAs: 'blob'
				}).then(function (response) {
					assert.strictEqual(response.constructor, Blob);
				});
			},

			'ArrayBuffer': function () {
				if (!hasResponseType) {
					this.skip('No responseType to test');
				}

				return xhr.get('/__services/request/xhr/responseTypeGif', {
					handleAs: 'arraybuffer'
				}).then(function (response) {
					assert.strictEqual(response.constructor, ArrayBuffer);
				});
			},

			'ArrayBuffer POST': function () {
				if (!hasResponseType) {
					this.skip('No responseType to test');
				}

				return xhr.post('/__services/request/xhr/responseTypeGif', {
					handleAs: 'arraybuffer'
				}).then(function (response) {
					assert.strictEqual(response.constructor, ArrayBuffer);
				});
			},

			'document': function () {
				if (!hasResponseType) {
					this.skip('No responseType to test');
				}

				return xhr.get('/__services/request/xhr/responseTypeDoc', {
					handleAs: 'document'
				}).then(function (response) {
					assert.strictEqual(response.constructor, document.constructor);
				});
			},

			'document POST': function () {
				if (!hasResponseType) {
					this.skip('No responseType to test');
				}

				return xhr.post('/__services/request/xhr/responseTypeDoc', {
					handleAs: 'document'
				}).then(function (response) {
					assert.strictEqual(response.constructor, document.constructor);
				});
			}
		}
	});
});
