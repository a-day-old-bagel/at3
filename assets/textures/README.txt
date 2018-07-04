Any file placed in this directory or a child directory, and having the extension .ktx, will be automatically loaded
into the engine when it is run. This behavior is probably just a temporary thing for development's sake.

NOTE: If a .ktx file has a name (the part preceding ".ktx") consisting of two or fewer characters, it will be loaded
using NEAREST filtering instead of LINEAR filtering. This is also for testing and development purposes, so if you don't
want your texture to look pixelated, name it something longer than two characters for now.
For example, the "0.ktx" file is a debug texture that gets applied to objects when no other texture is specified.
It's an 8x8 checkerboard pattern, and is appropriate for use with NEAREST filtering.
