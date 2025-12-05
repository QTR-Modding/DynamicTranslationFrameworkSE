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
| `keywords` | Array of strings | Yes | Array of keyword names |
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
  "keywords": ["MyKeyword", "0x01234567"],
  "dll": "MyLoreProvider.dll",
  "function": "GetLoreText"
}
```

**Multiple providers (array format):**

```json
[
  {
    "keywords": ["BookKeyword"],
    "dll": "BookLore.dll",
    "function": "GetBookLore"
  },
  {
    "keywords": ["WeaponKeyword", "ArmorKeyword"],
    "papyrus": "MyPapyrusScript",
    "function": "GetItemDescription"
  }
]
```

### Keyword Formats

Keywords can be specified in multiple formats:

1. **EditorID**: The string name of the keyword (e.g., `"VendorItemArmor"`)
2. **Hex FormID**: Prefixed with `0x` (e.g., `"0x0010F2D9"`)
3. **Decimal FormID**: Plain number as string (e.g., `"1110745"`)
4. **Plugin reference**: Format `0xFormID~PluginName.esp` (e.g., `"0x800~MyMod.esp"`)

### Native DLL Provider

For native DLL providers, the exported function must have this signature:

```cpp
extern "C" __declspec(dllexport) const wchar_t* __cdecl GetLoreText(RE::TESForm* item, RE::TESForm* owner);
```

The function receives:
- `item`: The form of the item being examined
- `owner`: The owner of the item (container, actor, etc.)

Returns a wide string with the lore text to display, or `nullptr`/empty string to skip.

### Logging

The configuration loader logs the following:
- Config folder discovery
- Each JSON file processed
- Successfully registered providers
- Errors for missing files, parse errors, failed keyword resolution, or missing functions
