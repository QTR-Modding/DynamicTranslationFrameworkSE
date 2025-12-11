#### YOU NEED CMAKE < 3.5 !

#### WINDOWS ENVIRONMENT VARIABLES TO SET

1. **`COMMONLIB_SSE_FOLDER`**: The path to your clone of Commonlib.
2. **`VCPKG_ROOT`**: The path to your clone of [vcpkg](https://github.com/microsoft/vcpkg).
3. (optional) **`SKYRIM_FOLDER`**: path of your Skyrim Special Edition folder.
4. (optional) **`SKYRIM_MODS_FOLDER`**: path of the folder where your mods are.

#### THINGS TO EDIT

1. CMakeLists.txt
- **`AUTHORNAME`**
- **`MDDNAME`**
- (optional) Your plugin version. Default: `0.1.0.0`
2. vcpkg.json
- **`name`**: Your plugin's name.
- **`version-string`**: Your plugin version. Default: `0.1.0.0`

#### FEATURES
Automatically imports:
- [CLibUtil](https://github.com/powerof3/CLibUtil) by powerof3
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) by Thiago099

---

## JSON Configuration

DynamicTranslationFrameworkSE reads provider definitions from JSON files in:

```
Data/SKSE/Plugins/DynamicTranslationFramework/
```

Files are loaded from the top level of this directory (subfolders are **not** scanned).
The parsing logic uses the [CLibUtilsQTR](https://github.com/QTR-Modding/CLibUtilsQTR)
preset helpers that back the `Presets::Field` types in `ConfigEntryBlock`.

### JSON Schema

Each JSON file can contain either a single provider object or an array of provider objects.

#### Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `strings` | Array of strings | Yes | Array of translation strings without the $ prefix. |
| `skse` | String | No | Name of the native DLL. `OnDynamicTranslationRequest` is resolved from this module. |
| `papyrus` | String | No | Editor ID of the form that has a Papyrus script attached. The script must expose `OnDynamicTranslationRequest`. |

Both native and Papyrus providers are optional, but at least one must be present. When both
are supplied the native call is attempted first; Papyrus is used as a fallback.

### Example Configuration

**Single provider (object format):**

```json
{
  "strings": ["LoreBox_quantAoT", "GeneralPage"],
  "skse": "MyProvider.dll"
}
```

**Multiple providers (array format):**

```json
[
  {
    "strings": ["GeneralPage"],
    "skse": "MyProvider.dll"
  },
  {
    "strings": ["Lockpicking", "Smithing"],
    "papyrus": "LoreBoxQuest"
  }
]
```
### Native DLL Provider

For native DLL providers, the exported function must have this signature:

```cpp
extern "C" __declspec(dllexport) const wchar_t* __cdecl OnDynamicTranslationRequest(std::string_view key);
```

The loader resolves `OnDynamicTranslationRequest` automatically after loading the DLL.
Return a wide string with the text to display, or `nullptr`/empty string to skip.

### Papyrus Provider

Papyrus entries reference the **Editor ID** of a form with an attached script that exposes
`OnDynamicTranslationRequest`. The current loader resolves Papyrus providers **only by
Editor ID**—numeric form IDs or raw FormIDs are not supported. At runtime the framework
looks up the form with `LookupByEditorID`, fetches the script via CLibUtilsQTR's Papyrus
helpers, and invokes the function with the translation key. The function should return a
non-empty string to override the translation.

### Logging

The configuration loader logs the following:
- Config folder discovery
- Each JSON file processed
- Successfully registered providers
- Errors for missing files, parse errors or missing functions
