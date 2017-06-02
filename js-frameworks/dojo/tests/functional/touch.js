define([
	'require',
	'intern!object',
	'intern/chai!assert'
], function (require, registerSuite, assert) {
	/* globals tracker */

	function loadPage(driver) {
		return driver.get('remote')
			.setExecuteAsyncTimeout(5000)
			.get(require.toUrl('./support/touch.html'));
	}

	function clickElement(element) {
		return element.click();
	}

	function tapElement(element) {
		return element.tap();
	}

	function moveMouseToElement(session, id) {
		return function () {
			return session.findById(id).then(function (element) {
				return session.moveMouseTo(element);
			}).end();
		};
	}

	registerSuite(function () {
		var tapOrClick;

		return {
			name: 'dojo/touch',

			'before': function () {
				// Not all browsers or drivers support touch events
				tapOrClick = this.get('remote').environmentType.touchEnabled ?
					tapElement :
					clickElement;
			},

			'beforeEach': function () {
				return loadPage(this);
			},

			'press': function () {
				var elementPromise = this.get('remote')
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							window.tracker = sinon.stub();
							var node = document.getElementById('upper-right');
							on(node, touch.press, tracker);
							done();
						});
					})
					.findById('upper-right');

				return tapOrClick(elementPromise)
					.execute(function () {
						return tracker.calledOnce;
					})
					.then(function (result) {
						assert.isTrue(result);
					});
			},

			'move': function () {
				var session = this.get('remote');

				return session
					.then(moveMouseToElement(session, 'upper-left'))
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							window.tracker = sinon.stub();
							var node = document.getElementById('upper-right');
							on(node, touch.move, tracker);
							done();
						});
					})
					.execute(function () {
						return tracker.callCount;
					})
					.then(function (result) {
						assert.equal(result, 0);
					})
					.then(moveMouseToElement(session, 'upper-right'))
					.execute(function () {
						return tracker.called;
					})
					.then(function (result) {
						assert.ok(result);
					});
			},

			'release': function () {
				return this.get('remote')
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							var node = document.getElementById('upper-left');
							window.tracker = sinon.stub();
							on(node, touch.release, tracker);
							done();
						});
					})
					.findById('upper-left')
					.click()
					.execute(function () {
						return tracker.called;
					})
					.then(function (result) {
						assert.isTrue(result);
					});
			},

			'cancel': function () {
				var session = this.get('remote');

				return this.get('remote')
					.then(moveMouseToElement(session, 'upper-left'))
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							var node = document.getElementById('upper-left');
							window.tracker = sinon.stub();
							on(node, touch.cancel, tracker);
							done();
						});
					})
					.then(moveMouseToElement(session, 'upper-right'))
					.then(moveMouseToElement(session, 'upper-left'))
					.execute(function () {
						return tracker.called;
					})
					.then(function (result) {
						assert.isTrue(result);
					});
			},

			'over': function () {
				var session = this.get('remote');

				return session
					.then(moveMouseToElement(session, 'upper-left'))
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							var node = document.getElementById('upper-right');
							window.tracker = sinon.stub();
							on(node, touch.over, tracker);
							done();
						});
					})
					.then(moveMouseToElement(session, 'upper-right'))
					.execute(function () {
						return tracker.called;
					})
					.then(function (result) {
						assert.isTrue(result);
					});
			},

			'out': function () {
				var session = this.get('remote');

				return session
					.then(moveMouseToElement(session, 'upper-left'))
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							var node = document.getElementById('upper-left');
							window.tracker = sinon.stub();
							on(node, touch.out, tracker);
							done();
						});
					})
					.then(moveMouseToElement(session, 'upper-right'))
					.execute(function () {
						return tracker.called;
					})
					.then(function (result) {
						assert.isTrue(result);
					});
			},

			'enter': function () {
				var session = this.get('remote');

				return session
					.then(moveMouseToElement(session, 'upper-left'))
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							var node = document.getElementById('upper-right');
							window.tracker = sinon.stub();
							on(node, touch.enter, tracker);
							done();
						});
					})
					.then(moveMouseToElement(session, 'upper-right'))
					.execute(function () {
						return tracker.calledOnce;
					})
					.then(function (result) {
						assert.isTrue(result);
					});
			},

			'leave': function () {
				var session = this.get('remote');

				return session
					.then(moveMouseToElement(session, 'upper-left'))
					.executeAsync(function (done) {
						require(['sinon', 'dojo/on', 'dojo/touch'], function (sinon, on, touch) {
							var node = document.getElementById('upper-left');
							window.tracker = sinon.stub();
							on(node, touch.leave, tracker);
							done();
						});
					})
					.then(moveMouseToElement(session, 'upper-right'))
					.execute(function () {
						return tracker.calledOnce;
					})
					.then(function (result) {
						assert.isTrue(result);
					});
			}
		};
	});
});
