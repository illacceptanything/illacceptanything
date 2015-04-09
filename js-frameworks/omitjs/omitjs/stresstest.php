<? include('omit.php');?>
<script type="text/javascript">
	/**
	 * The purpose of this file is to test so all omit-functionality is working correctly, specifically
	 * the references between objects.
	 */
	var o = {
		objA: {
			html: function(content) {
				return div(this, 'objA<br>this.obj = ' + this.obj +
						'<br>this.objName = '+ this.objName +
						'<br>content = '+ content +
						'<br>this.childA:<br>'+
						this.childA('argu1', 'argu2')+
						this.childB()
				);
			},
			css: function() {
				return this.obj+css({
					'c':'red'
				});
			},
			js: function() {
				var self = this;
				$(this.obj).offOn('click', function() {
					alert('objA.js:\nself.variable = ' + self.variable);
					self.hello();
				});
			},
			functions: {
				hello: function() {
					alert('function hello\nthis.obj = ' + this.obj +'\nthis.objName = '+ this.objName);
				},
				variable: '{var variable of objA\'s functions}',
				integer: 39
			},
			children: {
				childA: {
					html: function(arg1, arg2) {
						return '<div'+this.obj +'>childA'+
								'<br>this.objName = ' + this.objName +
								'<br>this.parent.obj = ' + this.parent.obj +
								'<br>arg1 = '+ arg1 +
								'<br>arg2 = '+ arg2+'</div>';
					},
					css: function() {
						return this.obj+'{trf:scale(3);b:2. solid black;pos:abs;top:300.;l:400.;}'
					},
					js: function() {
						var self = this;
						$(this.obj).offOn('click', function(e) {
							e.stopPropagation();
							self.func();
							alert('childA.js:\nself.obj = '+self.obj +'\nself.objName = '+ self.objName +'\nself.parent.objName = ' + self.parent.objName);
							alert('now call parent "hello" func');
							self.parent.hello();
						});
					},
					functions: {
						func: function() {
							alert('childA func()\nthis.name = ' + this.objName +'\nthis.obj = '+ this.obj);
						}
					}
				},
				childB: {
					html: function() {
						return div(this, 'name in childB.')
					},
					css: function() {
						return this.obj+'{c:white;bg:black;p:15.;}';
					},
					js: function() {
						var self = this;
						$(this.obj).offOn('click', function(){
							alert('call childA.func():');
							self.parent.childA.func();
						})
					}
				}
			}
		}
	}

	var page = {
		index: function() {
			return 'Hello!';
		}
	};

	var site = {
		title: 'Omit JS Stresstest',
		css: 'body{f-f:Arial;}',
		structure: function(){
			return o.objA('content :)');
		}
	};
</script>
