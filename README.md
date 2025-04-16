# Little Engine That Could

## Introduction
Little baby "engine" made for rapid prototyping for small contained projects.
The hope is not to be a full blown abstraction layer over Vulkan, since that
would be a crazy endeavour, instead just to be enough to cover the boilerplate.

## Development
- Basic rendering using buffer objects
- Somewhat usable interface for descriptor managment.

## TODO!
- Rework literally everything :)

## Engine Philosophy
Moving forward, I want to put a constrain on how I design the interface.
Since the whole point of a GPU API is to make the GPU do "stuff", I want
to revolve all the interfaces for that one and only purpose. As an example,
currently I can freely set the layout of the resources on a dime, and that
seemed like a great idea while developing the engine, however I have realized
that using shader reflection, I can simply only pull layouts from them, and 
never expose an interface to manually create layouts myself.
TL;DR: Feed the shaders, thats all that matters
