# Local SiN weapon scripts (partial R2 groundwork)

The independently authored `sin_weapon_scripts` C++17 library parses the owner's original `scripts/weapon_magnum.txt`, `scripts/weapon_assault_rifle.txt`, and `scripts/weapon_scattergun.txt` to construct `WeaponConfig`. No game assets are stored in this repository.

## Usage

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/sin_weapon_script_check "/path/to/SE1"
```

Pass the game directory containing `scripts/`, not the scripts subdirectory. On Windows with Visual Studio use `build\\Debug\\sin_weapon_script_check.exe`.

Fields: `viewmodel`, `ZoomFOV`, `clip_size`, `clip2_size`, `primary_ammo`, `secondary_ammo`, and optional `viewmodelOriginOffset`. Parser handles case-insensitive keys, comments, nested blocks, quotes and UTF-8 BOM. It validates capacities, numeric vector and FOV, rejects malformed script syntax and files over 1 MiB. It does not implement the entire KeyValues dialect.

Locally tested with the original SiN scripts: Magnum zoom=50 clip=20, Assault Rifle zoom=40 clip=45, Scattergun zoom=60 clip=6. Offsets are -0.052/0/-0.1, 0/0/0, and 0.115/0/0 respectively. These are viewmodel origin offsets, not validated VR hand alignment.

**Limitations:** does not load models or animations, register engine ammo types, provide a SourceVR module ABI, build an Android library, or run the game in VR. A real SourceVR engine adapter is not yet available. CTest uses synthetic scripts; the original game data remains local.
