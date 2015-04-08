# illacceptanything

> The project where literally* anything goes

I want to make a really cool project, but I don't know what to make. So I'll just accept
every Pull Request submitted and see what happens.

# Building
You like Docker, right? So we've provider a Dockfile which installs 
all dependencies needed to run everything in here in a container! Just `cd` into this
directory, then run `docker build .`. You should, once it finishes, be able to see
a set of new images in the output of `docker images`. 
Now, you can open a shell in this container with `docker run -i -t <containerID> /bin/bash` where
`<containerID>` is an identifying part of the final Image ID built by this container, as
output by `docker images`. Fancy!

\* No porn. Nothing illegal. Can't violate GitHub terms of service. Don't be a dick.

![](https://i.imgur.com/ehUtz.gif)
