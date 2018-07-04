Any file in this directory or in a child directory with the extension ".dae" will be automatically loaded into the
engine when it starts. This behavior is probably temporary, and is just meant to help during development.

NOTE: If the filename (the part preceding ".dae") ends with the word "Terrain" (case sensitive), the mesh, upon loading,
will also be used to generate a btBvhTriangleMeshShape for use as static terrain in the engine. This can be costly, so
if you don't want your mesh to be used as a static terrain, don't call it anything ending with the word "Terrain."
