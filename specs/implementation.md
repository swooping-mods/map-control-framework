# Map Control Framework: behavior and implementation

Describes the exported implementation **0.1.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Registers up to eight provider rows in the native map’s top section. Each provider describes up to 64 stable unit IDs and six actions; selection, labels, enabled states and commands remain provider-owned. The framework supplies native-style buttons, previous/next selection, status and retained native ground picks. Pointer capture runs before native map steering, suppressing panel-started drags/releases so a click cannot leak to markers underneath.

## State and lifecycle

Rows, selected IDs, picks and rooted UI belong to the current world/walker/map. Focus, typing, menus, restoration and active-map gates control display and dispatch. Context is revalidated when commands run; callbacks cannot retain borrowed pointers beyond their contract. World/map transitions destroy owned UI and reset captured input.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/map-control-framework/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Provides `map-control-framework.map-controls` | 1 | 32 bytes |

## Implementation sources

- [mod.c](../source/mods/map-control-framework/mod.c)
- [registry.h](../source/mods/map-control-framework/registry.h)
- [ground_pick.h](../source/mods/map-control-framework/ground_pick.h)
- [screen_gates.h](../source/mods/map-control-framework/screen_gates.h)

## Verification boundary

It does not grant authority, own robot/vehicle saves or provide pathfinding. Consumers must independently recheck permission and eligibility. Layout/input ordering and each provider’s actions need game qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
