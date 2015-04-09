<? include('omit.php');?>
<script type="text/javascript">
	var o = {
		wrap: {
			html: function(content) {
				return div(this, content);
			},
			css: function() {
				return this.obj+'{w:920.;m:100. auto;anim:'+animation.fadeIn+';}';
			}
		},
		object: {
			html: function(content) {
				return '<div'+this.obj+'>' + content + '</div>';
			},
			js: function() {
				var self = this;
				$(self.obj).offOn('click',function(){
					alert(self.obj);
				});
			}
		},
		header: {
			html: function() {
				return div(this,'<a href="http://www.omitjs.com">Omit.js</a> Boilerplate Header');
			},
			css: function() {
				return this.obj+'{f-s:20.;f-w:bold;m-b:100.;}';
			}
		},
		footer: {
			html: function() {
				return div(this,'<a href="http://www.omitjs.com">Omit.js</a> Boilerplate Footer')
			},
			css: function() {
				return this.obj+css({
					'f-s':'12.',
					'c':'gray',
					'm-t': '100.'
				});
			}
		}
	};

	var page = {
		index: function() {
			return o.object('Omit JS Boilerplate content. This boilerplate contains some elements that is commonly used on a website.');
		}
	};

	var animation = {
		fadeIn: {
			0: 'op:0;',
			100: 'op:1;',
			duration: 2500,
			iteration: 1,
			easing: 'ease-in'
		}
	};

	var site = {
		title: 'Omit JS Boilerplate',
		css: 'body{f-f:Arial;}',
		structure: function(){
			return o.wrap(o.header() + Omit.page + o.footer());
		}
	};
</script>
