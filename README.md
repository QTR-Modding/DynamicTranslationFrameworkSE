## What is DynamicTranslationFrameworkSE?

A lightweight add-on that teaches Skyrim to fetch new text when a `$LocalizationStrings` key appears on screen. When the game asks for a line like `$CraftingMenu`, the framework asks your chosen provider for the right wording and shows that instead.

It is meant for players and mod authors who want flexible in-game translations without shipping a full set of localized text files.

## Quick setup

1. Install the SKSE plugin like any other mod.
2. Create a folder in your Skyrim install: `Data/SKSE/Plugins/DynamicTranslationFramework/`.
3. Drop one or more JSON files in that folder to tell the framework where to get translations. Files in subfolders are ignored.

## How the providers work

* Each JSON file can describe one provider or a list of providers.
* A provider declares the `$LocalizationStrings` keys it can answer and where the replacement text comes from.
* Two source types exist:
  * **Native SKSE plugin** – a DLL that returns the text when asked.
  * **Papyrus form** – a form in your load order with a script that knows how to respond.
* If both are listed, the native plugin is asked first and the Papyrus script is tried if the native call gives no answer.

### Provider fields in plain language

| Field | Required | What to put here |
|-------|----------|------------------|
| `strings` | Yes | The list of `$LocalizationStrings` keys this provider can handle, without the `$` prefix. |
| `skse` | No | The name of the SKSE DLL that should supply the translated text. |
| `papyrus` | No | The Editor ID of the form whose attached Papyrus script can supply the text. Only Editor IDs are supported (numeric FormIDs do not work). |

### Examples

**Single provider**

```json
{
  "strings": ["LoreBox_quantAoT", "GeneralPage"],
  "skse": "MyProvider.dll"
}
```

**Multiple providers in one file**

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

### What happens in game

1. The framework scans `Data/SKSE/Plugins/DynamicTranslationFramework/` when the game starts.
2. When a `$LocalizationStrings` key is about to be shown, the framework checks your providers to see who promised that key.
3. If a match is found, it asks the native DLL first. If that does not provide text, it asks the Papyrus form by Editor ID.
4. If a provider returns a non-empty string, that is what the player sees on screen. If nobody responds, the usual `$KeyName` appears.

### Friendly tips

* Keep your JSON files at the top level of the folder—nested folders are skipped.
* Make sure the SKSE DLL or Papyrus form you name is installed and enabled in the load order.
* Use clear Editor IDs for Papyrus sources; numeric FormIDs are ignored.
* The framework reports which files it loaded and any problems it found in the SKSE log.
