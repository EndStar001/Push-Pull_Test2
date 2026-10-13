import unreal


ASSET_PATH = "/Game/UI_PushPull"
ASSET_NAME = "UI_PushPull"
PACKAGE_PATH = "/Game"


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", unreal.PushPullWidget)
        asset = asset_tools.create_asset(ASSET_NAME, PACKAGE_PATH, None, factory)

    unreal.EditorAssetLibrary.save_loaded_asset(asset)


main()
