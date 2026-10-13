# PushableActor

Unreal Engine project for a pushable actor workflow, including runtime and editor modules.

## Project Layout

- `Config/` - Unreal project configuration.
- `Content/` - Unreal maps, blueprints, meshes, textures, and other assets.
- `Source/PushableActor/` - Runtime module.
- `Source/PushableActorEditor/` - Editor module and asset tooling.
- `PushableActor.uproject` - Unreal project descriptor.

## Git Notes

This repository uses Git LFS for Unreal binary assets such as `.uasset` and `.umap` files.

Generated Unreal folders such as `Binaries/`, `Intermediate/`, `Saved/`, and `DerivedDataCache/` are intentionally ignored.
