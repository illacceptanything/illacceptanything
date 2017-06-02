Start the Mojito application;

> mojito start

Then view in a browser;

http://localhost:8666/

You can click on the text.

===

Make a HTML5 Application

From the Mojito application directory;

> mojito build html5app
> cd ./artifacts/builds/html5app/
> mojito start

Then view in a browser;

http://localhost:8666/

You can click on the text.

===

Make an iPad Application

Note: You must have XCode installed.

From the Mojito application directory;

> mojito create project xcode ipad
> open ./artifacts/projects/xcode/ipad/

Double click on the "mojito-ios.xcodeproj" icon. Once the project has open click the "Build & Run" icon in XCode.

Then view in the simulator; you can click on the text.