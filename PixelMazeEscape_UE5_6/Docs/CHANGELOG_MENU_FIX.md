# Main menu visibility fix

The native UMG hierarchy was previously created in `NativeConstruct()`. At that point Unreal had already called `RebuildWidget()`, so the Slate hierarchy remained empty and nothing was drawn.

The menu is now built in `NativeOnInitialized()` with an additional `RebuildWidget()` fallback. The player controller also logs successful widget creation or a configuration error.

## Expected log

```text
Pixel Maze Escape: Main menu added to viewport.
```
