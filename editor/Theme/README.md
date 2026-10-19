# Studio appearance

Studio offers **Follow System**, **Dark** and **Light** in Edit > Settings and
View > Theme. Settings selections preview immediately. Save commits the choice;
Cancel, Escape and closing the settings dialog restore the committed choice.
Returning to Follow System resolves the current OS appearance again.

`theme_mode` in the existing user `flowcv_editor.cfg` is `system`, `dark` or
`light`. A new config defaults to System. Existing configs without this key
retain Dark. Invalid values fall back to Dark independently of other settings.
Only the preference is persisted; effective appearance is never project data.

`ThemeManager` owns the application palette, shared stylesheet, Qt color-scheme
hint and QtNodes styles. Initialize it on the GUI thread before constructing
windows. Qt 6.8 or later is required. A system scheme of Unknown preserves the
last effective theme, initially Dark. Reapplying an unchanged effective theme
does not repaint the UI. Changing the preference still emits `modeChanged`.

The editor responds to `themeChanged` by resetting the view background cache
and invalidating item caches, without rebuilding nodes or changing selection,
positions, graph state or AutoTick. The public QtNodes style APIs are used;
the vendored node editor does not require modifications.

## Widgets and plugins

Standard Qt widgets inherit the application palette and QSS, including floating
windows and menus without a parent. Use palette roles for custom painting.
`ThemeColors.h` is header-only and available through `VisionFlow::QtNodeUi`:

```cpp
#include <Theme/ThemeColors.h>
const auto colors = VisionFlow::themeColors(widget->palette());
```

For custom widgets, handle `QEvent::PaletteChange` to invalidate cached colors
or simply read the palette on each paint. Avoid a private ThemeManager singleton
inside a plugin DLL. The host owns the manager and standard Qt palette/style
events propagate to plugin widgets.

Error and warning labels can set `severity` to `error` or `warning` before being
shown. When changing this property dynamically on a visible widget, call
`style()->unpolish(widget)`, `style()->polish(widget)` and `widget->update()`.
Color swatches use their actual color and choose a readable foreground locally.

Image pixels, algorithm overlay colors and saved 3D background parameters are
content and are not transformed by the UI theme. The image surround remains
neutral dark gray; an empty image viewer follows the application palette.

## Verification

With `VISIONFLOW_BUILD_STUDIO=ON` and `VISIONFLOW_BUILD_TESTS=ON`, build
`VisionFlowStudio` and `theme_test`, then run:

```powershell
ctest --test-dir out/build/local-x64-release -R "^theme$" --output-on-failure
```

The test uses the real Studio UI and ImageLoader plugin. It checks system
resolution via an injected reader, explicit override, Unknown fallback,
idempotence, palette contrast, resources, config migration, preview/save/cancel,
save failures, menu persistence, existing/new nodes and a running flow.
Dark/light render previews are written to the build's `theme-previews` directory.
Run the existing integration suite as well to cover other node UI providers.
Native title bars and OS-owned dialogs should also be inspected on the target OS;
Qt documents color-scheme override as a platform-dependent hint.
