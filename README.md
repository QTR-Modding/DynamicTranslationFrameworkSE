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

DynamicTranslationFrameworkSE supports loading provider definitions from JSON files located under:

```
Data/SKSE/Plugins/DynamicTranslationFrameworkSE/
```

The loader recursively scans this folder and all subfolders for `.json` files.

### JSON Schema

Each JSON file can contain either a single provider object or an array of provider objects.

#### Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `strings` | Array of strings | Yes | Array of translation strings without the $ prefix |
| `dll` | String | No | Name of the DLL exporting the native provider function |
| `papyrus` | String | No | Papyrus class name |
| `function` | String | Conditional | Function name (required if `dll` or `papyrus` is specified) |

A provider entry may be:
- **Native-only**: `dll` + `function` specified
- **Papyrus-only**: `papyrus` + `function` specified
- **Both**: `dll` and `papyrus` both specified

### Example Configuration

**Single provider (object format):**

```json
{
  "strings": ["LoreBox_quantAoT", "GeneralPage"],
  "dll": "MyProvider.dll",
  "function": "GetText"
}
```

**Multiple providers (array format):**

```json
[
  {
    "strings": ["GeneralPage"],
    "dll": "MyProvider.dll",
    "function": "GetText"
  },
  {
    "strings": ["Lockpicking", "Smithing"],
    "papyrus": "MyPapyrusScript",
    "function": "GetDescription"
  }
]
```
### Native DLL Provider

For native DLL providers, the exported function must have this signature:

```cpp
extern "C" __declspec(dllexport) const wchar_t* __cdecl GetText(RE::TESForm* item, RE::TESForm* owner);
```

The function receives:
- `item`: The form of the item being examined
- `owner`: The owner of the item (container, actor, etc.)

Returns a wide string with the text to display, or `nullptr`/empty string to skip.

### Logging

The configuration loader logs the following:
- Config folder discovery
- Each JSON file processed
- Successfully registered providers
- Errors for missing files, parse errors or missing functions
