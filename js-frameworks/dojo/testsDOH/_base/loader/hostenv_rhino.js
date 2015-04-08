dojo.provide("dojo.testsDOH._base._loader.hostenv_rhino");

tests.register("testsDOH._base._loader.hostenv_rhino",
	[
		function getText(t){
			var filePath = dojo.moduleUrl("testsDOH._base._loader", "getText.txt");
			var text = (new String(readText(filePath)));
			//The Java file read seems to add a line return.
			text = text.replace(/[\r\n]+$/, "");
			t.assertEqual("dojo._getText() test data", text);
		}
	]
);
