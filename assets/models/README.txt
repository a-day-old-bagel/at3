Any file in this directory or in a child directory with the extension ".dae" will be automatically loaded into the
engine when it starts. This behavior is probably temporary, and is just meant to help during development.

NOTE: If the filename (the part preceding ".dae") starts with the word "terrain" (case sensitive), the mesh, upon
loading, will also be stored for other uses like generating a btBvhTriangleMeshShape for use as static terrain in the
engine. This can be costly, so if you don't want your mesh to be used as a static terrain, don't call it anything
starting with the word "terrain." Along the same lines, if you want your mesh to be used as a static terrain, make sure
you DO name it that way, or it won't work and you won't know why.
