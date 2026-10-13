# PushableActor

Unreal Engine project for a pushable actor workflow, including runtime and editor modules.

## Project Layout

- `Config/` - Unreal project configuration.
- `Content/` - Unreal maps, blueprints, meshes, textures, and other assets.
- `Source/PushableActor/` - Runtime module.
- `Source/PushableActorEditor/` - Editor module and asset tooling.
- `PushableActor.uproject` - Unreal project descriptor.

## Git Notes

Unreal project assets such as `.uasset` and `.umap` files are stored directly in the repository so GitHub source ZIP downloads can be opened without Git LFS.

Generated Unreal folders such as `Binaries/`, `Intermediate/`, `Saved/`, and `DerivedDataCache/` are intentionally ignored.

## Open From GitHub

After cloning or downloading the GitHub source ZIP, right-click `PushableActor.uproject` and choose **Generate Visual Studio project files**, or open it with Unreal Engine 5.5 and allow Unreal to rebuild the missing `PushableActor` modules.

The warning about missing `PushableActor` binaries is expected after cloning because compiled Unreal binaries are generated locally and are not committed.
